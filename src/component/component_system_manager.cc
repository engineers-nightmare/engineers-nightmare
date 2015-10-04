#include "component_system_manager.h"
#include "../particle.h"

sensor_comparator_component_manager comparator_man;
gas_production_component_manager gas_man;
light_component_manager light_man;
physics_component_manager physics_man;
relative_position_component_manager pos_man;
power_component_manager power_man;
power_provider_component_manager power_provider_man;
pressure_sensor_component_manager pressure_man;
renderable_component_manager render_man;
surface_attachment_component_manager surface_man;
switch_component_manager switch_man;
type_component_manager type_man;
door_component_manager door_man;
reader_component_manager reader_man;

#include <glm/gtc/random.hpp>


extern particle_manager *particle_man;


/* I have no clue how we're going to actually handle these */
const char * comms_msg_type_switch_state = "switch_state";
const char * comms_msg_type_pressure_sensor_1_state = "pressure_sensor_1_state";
const char * comms_msg_type_pressure_sensor_2_state = "pressure_sensor_2_state";
const char * comms_msg_type_sensor_comparison_state = "sensor_comparison_state";


void
tick_gas_producers(ship_space * ship)
{
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
tick_doors(ship_space * ship)
{
    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];

        /* doors require: powered */
        assert(power_man.exists(ce) || !"doors must be powerable");
        assert(pos_man.exists(ce) || !"doors must be positioned");

        auto power = power_man.get_instance_data(ce);
        auto position = pos_man.get_instance_data(ce);

        /* it's a power door, it's not going /anywhere/ without power */
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

                        auto data = clamp(msg.data, 0.f, 1.f);
                        door_man.instance_pool.desired_pos[i] = data > 0 ? 1.0f : 0.0f;
                    }
                }
            }
        }

        auto desired_state = door_man.instance_pool.desired_pos[i];
        auto in_desired_state = door_man.instance_pool.pos[i] == desired_state;
        /* TODO: magic number for quiescent power */
        *power.required_power = in_desired_state ? 1 : *power.max_required_power;

        auto delta = clamp(door_man.instance_pool.pos[i] - desired_state, -0.1f, 0.1f);
        door_man.instance_pool.pos[i] -= delta;

        /* did we just enter desired state? */
        if (desired_state == door_man.instance_pool.pos[i] && !in_desired_state) {
            auto s = desired_state ? surface_none : surface_door;

            auto pos = glm::ivec3(*position.position);

            /* todo: this has no support for rotation whatsoever */
            for (auto h = 0; h < 2; ++h) {
                auto ym = glm::ivec3(pos.x, pos.y - 1, pos.z);
                auto yp = glm::ivec3(pos.x, pos.y + 1, pos.z);

                auto chunk = ship->get_chunk_containing(pos);
                chunk->render_chunk.valid = false;
                chunk = ship->get_chunk_containing(ym);
                chunk->render_chunk.valid = false;
                chunk = ship->get_chunk_containing(yp);
                chunk->render_chunk.valid = false;

                /* we'll be calling ensure in set/remove surfaces anyway */
                auto bl = ship->ensure_block(pos);
                auto surfs = bl->surfs;

                if (s == surface_none) {
                    if (surfs[surface_yp] == surface_door) {
                        ship->remove_surface(pos, yp, surface_yp);
                    }

                    if (surfs[surface_ym] == surface_door) {
                        ship->remove_surface(pos, ym, surface_ym);
                    }
                }
                else {
                    if (surfs[surface_yp] == surface_none) {
                        ship->set_surface(pos, yp, surface_yp, s);
                    }

                    if (surfs[surface_ym] == surface_none) {
                        ship->set_surface(pos, ym, surface_ym, s);
                    }
                }

                mark_lightfield_update(pos);

                ++pos.z;
            }
        }
    }
}


void
tick_power_consumers(ship_space * ship) {
    for (auto i = 0u; i < power_man.buffer.num; i++) {
        auto ce = power_man.instance_pool.entity[i];

        auto & powered = power_man.instance_pool.powered[i];
        auto old_powered = powered;
        powered = false;

        auto & power_attaches = ship->entity_to_attach_lookups[wire_type_power];
        auto attaches = power_attaches.find(ce);
        if (attaches == power_attaches.end()) {
            continue;
        }

        std::unordered_set<unsigned> visited_wires;
        for (auto sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, wire_type_power, sea);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto const & wire = ship->power_wires[wire_index];

            visited_wires.insert(wire_index);
            /* todo: this needs to somehow handle multiple wires */
            if (wire.total_power >= wire.total_draw && wire.total_power > 0) {
                powered = true;
            }
        }

        if (powered != old_powered) {
            /* if a light changed power state, do the required update now */
            if (light_man.exists(ce)) {
                auto pos = *pos_man.get_instance_data(ce).position;
                auto block_pos = get_coord_containing(pos);
                mark_lightfield_update(block_pos);
            }
        }
    }
}


void
tick_light_components(ship_space* ship) {
    for (auto i = 0u; i < light_man.buffer.num; i++) {
        auto ce = light_man.instance_pool.entity[i];
        auto light_type = light_man.instance_pool.type[i];
        auto type = wire_type_comms;

        /* all lights currently require: position */
        assert(pos_man.exists(ce) || !"lights must have a position");
        assert(power_man.exists(ce) || !"lights must have power");

        auto power = power_man.get_instance_data(ce);
        auto light = light_man.get_instance_data(ce);

        auto & comms_attaches = ship->entity_to_attach_lookups[type];
        auto attaches = comms_attaches.find(ce);
        if (attaches != comms_attaches.end()) {

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

                    if ((light_type == 1 &&
                           (msg.desc == comms_msg_type_switch_state ||
                            msg.desc == comms_msg_type_pressure_sensor_1_state ||
                            msg.desc == comms_msg_type_pressure_sensor_2_state)) ||
                        (light_type == 2 && msg.desc == comms_msg_type_sensor_comparison_state)) {

                        auto data = clamp(msg.data, 0.f, 1.f);
                        *(light.requested_intensity) = data;

                        auto pos = *pos_man.get_instance_data(ce).position;
                        auto block_pos = get_coord_containing(pos);
                        mark_lightfield_update(block_pos);
                    }
                }
            }
        }

        if (!*power.powered) {
            *(light.intensity) = 0.0f;
        }
        else {
            *(light.intensity) = *(light.requested_intensity);
        }
        
        *(power.required_power) = *(light.requested_intensity) * *(power.max_required_power);

    }
}


void
tick_pressure_sensors(ship_space* ship) {
    for (auto i = 0u; i < pressure_man.buffer.num; i++) {
        auto ce = pressure_man.instance_pool.entity[i];

        /* all pressure sensors currently require: position */
        assert(pos_man.exists(ce) || !"pressure sensors must have a position");

        auto pos = *pos_man.get_instance_data(ce).position;

        glm::ivec3 pos_block = get_coord_containing(pos);

        topo_info *t = topo_find(ship->get_topo_info(pos_block));
        zone_info *z = ship->get_zone_info(t);
        float pressure = z ? (z->air_amount / t->size) : 0.0f;

        auto wire_type = wire_type_comms;
        auto & comms_attaches = ship->entity_to_attach_lookups[wire_type];
        auto attaches = comms_attaches.find(ce);
        if (attaches == comms_attaches.end()) {
            continue;
        }

        std::unordered_set<unsigned> visited_wires;
        for (auto sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, wire_type, sea);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto which_sensor = pressure_man.instance_pool.type[i];
            auto desc = comms_msg_type_pressure_sensor_1_state;
            if (which_sensor == 2) {
                desc = comms_msg_type_pressure_sensor_2_state;
            }
            visited_wires.insert(wire_index);

            comms_msg msg;
            msg.originator = ce;
            msg.desc = desc;
            msg.data = pressure;
            publish_message(ship, wire_index, msg);
        }
    }
}


void
tick_sensor_comparators(ship_space* ship) {
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
        visited_wires.clear();

        auto const & pub_attaches = comms_attaches[ce];
        for (auto sea : pub_attaches) {
            auto wire_index = attach_topo_find(ship, type, sea);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto desc = comms_msg_type_sensor_comparison_state;

            visited_wires.insert(wire_index);

            comms_msg msg;
            msg.originator = ce;
            msg.desc = desc;
            msg.data = difference;
            publish_message(ship, wire_index, msg);
        }
    }
}


void
draw_renderables(frame_data *frame)
{
    for (auto i = 0u; i < render_man.buffer.num; i++) {
        auto ce = render_man.instance_pool.entity[i];
        auto & mesh = render_man.instance_pool.mesh[i];
        auto & mat = *pos_man.get_instance_data(ce).mat;

        auto entity_matrix = frame->alloc_aligned<glm::mat4>(1);
        *entity_matrix.ptr = mat;
        entity_matrix.bind(1, frame);

        draw_mesh(mesh);
    }
}


void
draw_doors(frame_data *frame)
{
    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];
        auto & mesh = door_man.instance_pool.mesh[i];
        glm::mat4 mat = *pos_man.get_instance_data(ce).mat;

        auto pos = door_man.instance_pool.pos[i];

        mat[3][0] += pos * mat[0][0];
        mat[3][1] += pos * mat[0][1];
        mat[3][2] += pos * mat[0][2];

        auto entity_matrix = frame->alloc_aligned<glm::mat4>(1);
        *entity_matrix.ptr = mat;
        entity_matrix.bind(1, frame);

        draw_mesh(mesh);
    }
}
