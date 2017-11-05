#include <glm/gtc/random.hpp>
#include <memory>

#include "component_system_manager.h"
#include "../asset_manager.h"
#include "../particle.h"
#include "../mesh.h"

#define INITIAL_MAX_COMPONENTS 20

extern asset_manager asset_man;
extern component_system_manager component_system_man;

extern particle_manager *particle_man;

/* I have no clue how we're going to actually handle these */
const char *comms_msg_type_switch_state = "switch_state";
const char *comms_msg_type_pressure_sensor_1_state = "pressure_sensor_1_state";
const char *comms_msg_type_pressure_sensor_2_state = "pressure_sensor_2_state";
const char *comms_msg_type_sensor_comparison_state = "sensor_comparison_state";
const char *comms_msg_type_proximity_sensor_state = "proximity_sensor_state";


void
tick_gas_producers(ship_space *ship)
{
    auto &gas_man = component_system_man.managers.gas_producer_component_man;
    auto &power_man = component_system_man.managers.power_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;

    for (auto i = 0u; i < gas_man.buffer.num; i++) {
        auto ce = gas_man.instance_pool.entity[i];

        /* gas producers require: power */
        assert(power_man.exists(ce) || !"gas producer must be powerable");
        assert(pos_man.exists(ce) || !"gas producer must have position");

        auto power = power_man.get_instance_data(ce);
        auto position = pos_man.get_instance_data(ce);

        /* don't do anything if we aren't powered and turned on */
        if (!*power.powered) {
            continue;
        }

        auto comms = wire_type_comms;
        auto & comms_attaches = ship->entity_to_attach_lookups[comms];
        auto attaches = comms_attaches.find(ce);
        if (attaches != comms_attaches.end()) {
            std::unordered_set<unsigned> visited_wires;
            for (auto const & sea : attaches->second) {
                auto wire_index = attach_topo_find(ship, comms, sea);
                if (visited_wires.find(wire_index) != visited_wires.end()) {
                    continue;
                }

                auto const & wire = ship->comms_wires[wire_index];

                visited_wires.insert(wire_index);

                /* now that we have the wire, see if it has any msgs for us */
                /* todo: origin discrimination */
                for (auto msg : wire.read_buffer) {

                    if (msg.desc == comms_msg_type_switch_state) {

                        auto data = clamp(msg.data, 0.0f, 1.0f);
                        gas_man.instance_pool.enabled[i] = data > 0;
                        *power.required_power = data > 0 ? *power.max_required_power : 0.0f;
                    }
                }
            }
        }

        /* we are powered if we get here. check if turned on */
        if (!gas_man.instance_pool.enabled[i]) {
            continue;
        }

        auto pos = get_coord_containing(*position.position);

        /* topo node containing the entity */
        topo_info *t = topo_find(ship->get_topo_info(pos));
        zone_info *z = ship->get_zone_info(t);
        if (!z) {
            /* if there wasn't a zone, make one */
            z = ship->zones[t] = new zone_info(0);
        }

        /* add some gas if we can, up to our pressure limit */
        float max_gas = gas_man.instance_pool.max_pressure[i] * t->size;
        if (z->air_amount < max_gas) {
            z->air_amount = std::min(max_gas, z->air_amount + gas_man.instance_pool.flow_rate[i]);

            /* particle visibility is based on condensation/deposition process
             * as the gas enters a /much/ lower pressure environment.
             * we'll say it goes to zero at 0.3atm, and falloff is roughly quadratic
             */
            auto vis = std::max(0.0f, 0.3f - (float)z->air_amount / max_gas) / 0.3f;
            vis = vis * vis;

            if (vis > 0.0f) {
                /* emit some particles */
                auto mat = glm::mat3(*position.mat);
                for (auto j = 0; j < 5; j++) {
                    auto spawn_pos = *position.position
                            + 0.78f * glm::vec3(mat[2])
                            + glm::linearRand(0.25f * (mat[0] + mat[1]), 0.75f * (mat[0] + mat[1]));
                    spawn_pos.x = 0.1f * glm::round(spawn_pos.x / 0.1f);
                    particle_man->spawn(
                            spawn_pos,
                            0.01f * glm::vec3(mat[2]) +
                            glm::gaussRand(glm::vec3(0.0f), glm::vec3(0.005f)),
                            vis);
                }
            }
        }
    }
}

void
set_door_state(ship_space *ship, c_entity ce, surface_type s)
{
    auto &door_man = component_system_man.managers.door_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;

    auto position = pos_man.get_instance_data(ce);
    auto pos = glm::ivec3(*position.position);
    auto door = door_man.get_instance_data(ce);

    auto from_surface = s == surface_none ? surface_door : surface_none;

    /* todo: this has no support for rotation whatsoever */
    for (auto h = 0; h < *door.height; ++h) {
        auto ym = glm::ivec3(pos.x, pos.y - 1, pos.z);
        auto yp = glm::ivec3(pos.x, pos.y + 1, pos.z);

        /* we'll be calling ensure in set/remove surfaces anyway */
        auto bl = ship->ensure_block(pos);
        auto surfs = bl->surfs;

        if (surfs[surface_yp] == from_surface) {
            ship->set_surface(pos, yp, surface_yp, s);
        }

        if (surfs[surface_ym] == from_surface) {
            ship->set_surface(pos, ym, surface_ym, s);
        }

        ++pos.z;
    }
}

void
tick_doors(ship_space *ship)
{
    auto &pos_man = component_system_man.managers.relative_position_component_man;
    auto &door_man = component_system_man.managers.door_component_man;
    auto &reader_man = component_system_man.managers.reader_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];

        /* doors require: powered */
        assert(power_man.exists(ce) || !"doors must be powerable");
        assert(pos_man.exists(ce) || !"doors must be positioned");
        assert(reader_man.exists(ce) || !"doors must have reader");

        auto power = power_man.get_instance_data(ce);
        auto reader = reader_man.get_instance_data(ce);

        /* it's a power door, it's not going /anywhere/ without power */
        if (!*power.powered) {
            continue;
        }

        auto old_pos = door_man.instance_pool.pos[i];

        door_man.instance_pool.desired_pos[i] = *reader.data > 0 ? 1.0f : 0.0f;

        auto desired_state = door_man.instance_pool.desired_pos[i];
        auto in_desired_state = door_man.instance_pool.pos[i] == desired_state;
        /* TODO: magic number for quiescent power */
        *power.required_power = in_desired_state ? 1 : *power.max_required_power;

        auto delta = clamp(door_man.instance_pool.pos[i] - desired_state, -0.1f, 0.1f);
        door_man.instance_pool.pos[i] -= delta;

        auto pos = door_man.instance_pool.pos[i];

        if (desired_state == 0 && old_pos != 0 && pos == 0) {
            /* did we just finish closing? */
            set_door_state(ship, ce, surface_door);
        }
        else if (desired_state == 1 && old_pos == 0 && pos != 0) {
            /* just started opening? */
            set_door_state(ship, ce, surface_none);
        }
    }
}


void
tick_power_consumers(ship_space *ship) {
    auto power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < power_man.buffer.num; i++) {
        auto ce = power_man.instance_pool.entity[i];

        if (power_man.instance_pool.max_required_power[i] == 0 &&
            power_man.instance_pool.required_power[i] == 0)
            continue;
        power_man.instance_pool.powered[i] = false;

        auto & power_attaches = ship->entity_to_attach_lookups[wire_type_power];
        auto attaches = power_attaches.find(ce);
        if (attaches == power_attaches.end()) {
            continue;
        }

        for (auto sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, wire_type_power, sea);

            auto const & wire = ship->power_wires[wire_index];

            if (wire.total_power >= wire.total_draw && wire.total_power > 0) {
                power_man.instance_pool.powered[i] = true;
            }
        }
    }
}


void
tick_light_components(ship_space *ship) {
    auto &light_man = component_system_man.managers.light_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;
    auto &reader_man = component_system_man.managers.reader_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < light_man.buffer.num; i++) {
        auto ce = light_man.instance_pool.entity[i];

        /* all lights currently require: position, power, reader */
        assert(pos_man.exists(ce) || !"lights must have a position");
        assert(power_man.exists(ce) || !"lights must have power");
        assert(reader_man.exists(ce) || !"lights must have reader");

        auto power = power_man.get_instance_data(ce);
        auto light = light_man.get_instance_data(ce);
        auto reader = reader_man.get_instance_data(ce);

        *(light.requested_intensity) = clamp(*(reader.data), 0.0f, 1.0f);

        auto old_intensity = *(light.intensity);
        auto new_intensity = *power.powered ? *(light.requested_intensity) : 0.0f;

        if (old_intensity != new_intensity) {

            *(light.intensity) = new_intensity;
            *(power.required_power) = *(light.requested_intensity) * *(power.max_required_power);

//            auto pos = *pos_man.get_instance_data(ce).position;
//            auto block_pos = get_coord_containing(pos);
        }
    }
}


void
tick_pressure_sensors(ship_space* ship) {
    auto &pressure_man = component_system_man.managers.pressure_sensor_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;

    for (auto i = 0u; i < pressure_man.buffer.num; i++) {
        auto ce = pressure_man.instance_pool.entity[i];

        /* all pressure sensors currently require: position */
        assert(pos_man.exists(ce) || !"pressure sensors must have a position");

        auto pos = *pos_man.get_instance_data(ce).position;

        glm::ivec3 pos_block = get_coord_containing(pos);

        topo_info *t = topo_find(ship->get_topo_info(pos_block));
        zone_info *z = ship->get_zone_info(t);
        float pressure = z ? (z->air_amount / t->size) : 0.0f;

        auto which_sensor = pressure_man.instance_pool.type[i];
        auto desc = comms_msg_type_pressure_sensor_1_state;
        if (which_sensor == 2) {
            desc = comms_msg_type_pressure_sensor_2_state;
        }

        comms_msg msg;
        msg.originator = ce;
        msg.desc = desc;
        msg.data = pressure;

        publish_msg(ship, ce, msg);
    }
}


void
tick_sensor_comparators(ship_space *ship) {
    auto &comparator_man = component_system_man.managers.sensor_comparator_component_man;

    for (auto i = 0u; i < comparator_man.buffer.num; i++) {
        auto ce = comparator_man.instance_pool.entity[i];
        auto type = wire_type_comms;
        auto sensor_1 = FLT_MAX;
        auto sensor_2 = FLT_MAX;
        auto epsilon = comparator_man.instance_pool.compare_epsilon[i];
        auto & difference = comparator_man.instance_pool.compare_result[i];
        difference = 0.f;

        /* read pressure sensors from wire
         * bail after encountering first of each
         */
        auto & comms_attaches = ship->entity_to_attach_lookups[type];
        auto attaches = comms_attaches.find(ce);
        if (attaches == comms_attaches.end()) {
            continue;
        }

        std::unordered_set<unsigned> visited_wires;
        for (auto sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, type, sea);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto const & wire = ship->comms_wires[wire_index];

            visited_wires.insert(wire_index);

            /* now that we have the wire, see if it has any msgs for us */
            /* todo: origin discrimination */
            for (auto msg : wire.read_buffer) {
                if (sensor_1 == FLT_MAX && msg.desc == comms_msg_type_pressure_sensor_1_state) {
                    sensor_1 = msg.data;
                }
                if (sensor_2 == FLT_MAX && msg.desc == comms_msg_type_pressure_sensor_2_state) {
                    sensor_2 = msg.data;
                }

                if (sensor_1 != FLT_MAX && sensor_2 != FLT_MAX) {
                    break;
                }
            }
        }


        /* calculate difference */
        /* was epsilon chosen wisely? */

        /* the following code always returns 0??
         * difference = (fabsf(sensor_1 - sensor_2) < epsilon) ? 1.f : 0.f;
         */
        auto d = fabsf(sensor_1 - sensor_2);
        auto b = d < epsilon;
        difference = b ? 1.f : 0.f;

        /* publish result */
        comms_msg msg;
        msg.originator = ce;
        msg.desc = comms_msg_type_sensor_comparison_state;
        msg.data = difference;
        publish_msg(ship, ce, msg);
    }
}

void
tick_proximity_sensors(ship_space *ship, player *pl) {
    auto &proximity_man = component_system_man.managers.proximity_sensor_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < proximity_man.buffer.num; i++) {
        auto ce = proximity_man.instance_pool.entity[i];

        /* all proximity sensors currently require: position and power */
        assert(pos_man.exists(ce) || !"proximity sensors must have a position");
        assert(surface_man.exists(ce) || !"proximity sensors must have a surface");
        assert(power_man.exists(ce) || !"proximity sensors must have power");

        // Cannot detect or generate messages if the sensor isn't powered
        if (!*power_man.get_instance_data(ce).powered) {
            continue;
        }

        auto proximity = proximity_man.get_instance_data(ce);
        auto surface = surface_man.get_instance_data(ce);
        bool was_detected = *(proximity.is_detected);

        auto pos = *pos_man.get_instance_data(ce).position;
        glm::ivec3 sensor_pos_block = get_coord_containing(pos);
        glm::ivec3 player_pos_block = get_coord_containing(pl->pos);

        glm::vec3 delta = player_pos_block - sensor_pos_block;

        // Do quick relative range check first
        if (glm::length((delta)) <= *(proximity.range)) {

            glm::vec3 normal_ray;
            glm::vec3 entity_pos_offset;
            switch (*(surface.face)) {
            case surface_zp:
                entity_pos_offset = glm::vec3(0, 0, -1);
                normal_ray = glm::vec3(0.0f, 0.0f, -1.0f);
                break;
            case surface_zm:
                entity_pos_offset = glm::vec3(0, 0, 0);
                normal_ray = glm::vec3(0.0f, 0.0f, 1.0f);
                break;
            case surface_xp:
                entity_pos_offset = glm::vec3(-1, 0, 0);
                normal_ray = glm::vec3(-1.0f, 0.0f, 0.0f);
                break;
            case surface_xm:
                entity_pos_offset = glm::vec3(0, 0, 0);
                normal_ray = glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case surface_yp:
                entity_pos_offset = glm::vec3(0, -1, 0);
                normal_ray = glm::vec3(0.0f, -1.0f, 0.0f);
                break;
            case surface_ym:
                entity_pos_offset = glm::vec3(0, 0, 0);
                normal_ray = glm::vec3(0.0f, 1.0f, 0.0f);
                break;
            default:
                entity_pos_offset = glm::vec3(0, 0, 0);
                normal_ray = glm::vec3(0.0f, 0.0f, 0.0f);
                break;
            }

            // Check angle between player to sensor and sensor normal to make sure it is less than or equal to 90 degrees
            glm::vec3 player_to_sensor = pl->pos - pos + entity_pos_offset;
            *(proximity.is_detected) = glm::dot(normal_ray, player_to_sensor) >= 0;
        }
        else
        {
            *(proximity.is_detected) = false;
        }

        //Only publish the message if the sensor state changed
        if (was_detected != *(proximity.is_detected))
        {
            comms_msg msg;
            msg.originator = ce;
            msg.desc = comms_msg_type_proximity_sensor_state;
            msg.data = (*(proximity.is_detected)) ? 1.0f : 0.0f;
            publish_msg(ship, ce, msg);
        }
    }
}


void
tick_readers(ship_space *ship) {
    auto &reader_man = component_system_man.managers.reader_component_man;

    for (auto i = 0u; i < reader_man.buffer.num; i++) {
        auto ce = reader_man.instance_pool.entity[i];

        auto & comms_attaches = ship->entity_to_attach_lookups[wire_type_comms];
        auto attaches = comms_attaches.find(ce);
        if (attaches == comms_attaches.end()) {
            continue;
        }

        for (auto sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, wire_type_comms, sea);
            auto const & wire = ship->comms_wires[wire_index];

            for (auto msg : wire.read_buffer) {
                /* if we're filtering by source, and missed -- skip this one. */
                if (reader_man.instance_pool.source[i].id &&
                    reader_man.instance_pool.source[i].id != msg.originator.id) {
                    continue;
                }

                /* if we're filtering by desc, and missed -- skip */
                /* we /assume/ that everyone here has their strings interned. */
                if (reader_man.instance_pool.desc[i] &&
                    reader_man.instance_pool.desc[i] != msg.desc) {
                    continue;
                }

                reader_man.instance_pool.data[i] = msg.data;

                /* TODO: record /when/ we last got a matching packet */
            }
        }
    }
}

void
draw_renderables(frame_data *frame)
{
    auto &render_man = component_system_man.managers.renderable_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;

    for (auto i = 0u; i < render_man.buffer.num; i++) {
        auto ce = render_man.instance_pool.entity[i];
        auto & mesh_name = render_man.instance_pool.mesh[i];
        auto & mesh = asset_man.meshes[mesh_name];
        auto & mat = *pos_man.get_instance_data(ce).mat;

        auto entity_matrix = frame->alloc_aligned<glm::mat4>(1);
        *entity_matrix.ptr = mat;
        entity_matrix.bind(1, frame);

        draw_mesh(mesh.hw);
    }
}


void
draw_doors(frame_data *frame)
{
    auto &door_man = component_system_man.managers.door_component_man;
    auto &pos_man = component_system_man.managers.relative_position_component_man;

    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];
        auto & mesh_name = door_man.instance_pool.mesh[i];

        if (!mesh_name) {
            continue;
        }

        auto & mesh = asset_man.meshes[mesh_name];

        glm::mat4 mat = *pos_man.get_instance_data(ce).mat;

        auto pos = door_man.instance_pool.pos[i];

        mat[3][0] += pos * mat[0][0];
        mat[3][1] += pos * mat[0][1];
        mat[3][2] += pos * mat[0][2];

        auto entity_matrix = frame->alloc_aligned<glm::mat4>(1);
        *entity_matrix.ptr = mat;
        entity_matrix.bind(1, frame);

        draw_mesh(mesh.hw);
    }
}
