#include <glm/gtc/random.hpp>
#include <memory>

#include "component_system_manager.h"
#include "../asset_manager.h"
#include "../particle.h"
#include "../mesh.h"
#include "../entity_utils.h"

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
    auto &pos_man = component_system_man.managers.position_component_man;
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;

    for (auto i = 0u; i < gas_man.buffer.num; i++) {
        auto ce = gas_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto position = pos_man.get_instance_data(ce);

        /* don't do anything if we aren't powered and turned on */
        if (!*power.powered) {
            continue;
        }

        auto const &cwire = cwire_man.get_instance_data(ce);
        auto const &net = ship->get_comms_network(*cwire.network);
        /* now that we have the wire, see if it has any msgs for us */
        /* todo: origin discrimination */
        for (auto msg : net.read_buffer) {

            if (msg.desc == comms_msg_type_switch_state) {

                auto data = clamp(msg.data, 0.0f, 1.0f);
                gas_man.instance_pool.enabled[i] = data > 0;
                *power.required_power = data > 0 ? *power.max_required_power : 0.0f;
            }
        }

        /* we are powered if we get here. check if turned on */
        if (!gas_man.instance_pool.enabled[i]) {
            continue;
        }

        auto mat = glm::mat3(*position.mat);
        auto pos = glm::vec3((*position.mat)[3]);

        /* topo node containing the entity */
        topo_info *t = topo_find(ship->get_topo_info(get_coord_containing(pos)));
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

                for (auto j = 0; j < 5; j++) {
                    auto spawn_pos = pos
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
tick_doors(ship_space *ship)
{
    auto &door_man = component_system_man.managers.door_component_man;
    auto &reader_man = component_system_man.managers.reader_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto reader = reader_man.get_instance_data(ce);

        /* it's a power door, it's not going /anywhere/ without power */
        if (!*power.powered) {
            continue;
        }

        door_man.instance_pool.desired_pos[i] = *reader.data > 0 ? 1.0f : 0.0f;

        auto desired_state = door_man.instance_pool.desired_pos[i];
        auto in_desired_state = door_man.instance_pool.pos[i] == desired_state;
        /* TODO: magic number for quiescent power */
        *power.required_power = in_desired_state ? 1 : *power.max_required_power;

        auto delta = clamp(door_man.instance_pool.pos[i] - desired_state, -0.1f, 0.1f);
        door_man.instance_pool.pos[i] -= delta;
    }
}


void
tick_power_consumers(ship_space *ship) {
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < power_man.buffer.num; i++) {
        if (power_man.instance_pool.max_required_power[i] == 0 &&
            power_man.instance_pool.required_power[i] == 0)
            continue;

        auto const &net = ship->get_power_network(power_man.instance_pool.network[i]);
        power_man.instance_pool.powered[i] = net.total_power >= net.total_draw && net.total_power > 0;
    }
}


void
tick_light_components(ship_space *ship) {
    auto &light_man = component_system_man.managers.light_component_man;
    auto &reader_man = component_system_man.managers.reader_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < light_man.buffer.num; i++) {
        auto ce = light_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto light = light_man.get_instance_data(ce);
        auto reader = reader_man.get_instance_data(ce);

        *(light.requested_intensity) = clamp(*(reader.data), 0.0f, 1.0f);

        auto old_intensity = *(light.intensity);
        auto new_intensity = *power.powered ? *(light.requested_intensity) : 0.0f;

        if (old_intensity != new_intensity) {
            *(light.intensity) = new_intensity;
            *(power.required_power) = *(light.requested_intensity) * *(power.max_required_power);
        }
    }
}


void
tick_pressure_sensors(ship_space* ship) {
    auto &pressure_man = component_system_man.managers.pressure_sensor_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;

    for (auto i = 0u; i < pressure_man.buffer.num; i++) {
        auto ce = pressure_man.instance_pool.entity[i];
        assert(cwire_man.exists(ce));

        auto pos = glm::vec3((*pos_man.get_instance_data(ce).mat)[3]);
        auto network = *cwire_man.get_instance_data(ce).network;

        glm::ivec3 pos_block = get_coord_containing(pos);

        topo_info *t = topo_find(ship->get_topo_info(pos_block));
        zone_info *z = ship->get_zone_info(t);
        float pressure = z ? (z->air_amount / t->size) : 0.0f;

        auto which_sensor = pressure_man.instance_pool.type[i];
        auto desc = comms_msg_type_pressure_sensor_1_state;
        if (which_sensor == 2) {
            desc = comms_msg_type_pressure_sensor_2_state;
        }

        comms_msg msg{
            ce,
            desc,
            pressure,
        };
        publish_msg(ship, network, msg);
    }
}


void
tick_sensor_comparators(ship_space *ship) {
    auto &comparator_man = component_system_man.managers.sensor_comparator_component_man;
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;

    for (auto i = 0u; i < comparator_man.buffer.num; i++) {
        auto ce = comparator_man.instance_pool.entity[i];

        auto sensor_1 = FLT_MAX;
        auto sensor_2 = FLT_MAX;
        auto epsilon = comparator_man.instance_pool.compare_epsilon[i];
        auto & difference = comparator_man.instance_pool.compare_result[i];
        difference = 0.f;

        /* read pressure sensors from wire
         * bail after encountering first of each
         */
        auto const &cwire = cwire_man.get_instance_data(ce);
        auto net_id = *cwire.network;
        auto const &net = ship->get_comms_network(net_id);

        /* now that we have the wire, see if it has any msgs for us */
        /* todo: origin discrimination */
        for (auto msg : net.read_buffer) {
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


        /* calculate difference */
        /* was epsilon chosen wisely? */

        /* the following code always returns 0??
         * difference = (fabsf(sensor_1 - sensor_2) < epsilon) ? 1.f : 0.f;
         */
        auto d = fabsf(sensor_1 - sensor_2);
        auto b = d < epsilon;
        difference = b ? 1.f : 0.f;

        /* publish result */
        comms_msg msg{
            ce,
            comms_msg_type_sensor_comparison_state,
            difference,
        };
        publish_msg(ship, net_id, msg);
    }
}

void
tick_proximity_sensors(ship_space *ship, player *pl) {
    auto &proximity_man = component_system_man.managers.proximity_sensor_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;
    auto &surface_man = component_system_man.managers.surface_attachment_component_man;
    auto &power_man = component_system_man.managers.power_component_man;
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;

    for (auto i = 0u; i < proximity_man.buffer.num; i++) {
        auto ce = proximity_man.instance_pool.entity[i];

        // Cannot detect or generate messages if the sensor isn't powered
        if (!*power_man.get_instance_data(ce).powered) {
            continue;
        }

        auto proximity = proximity_man.get_instance_data(ce);
        auto surface = surface_man.get_instance_data(ce);
        bool was_detected = *(proximity.is_detected);

        auto pos = glm::vec3((*pos_man.get_instance_data(ce).mat)[3]);
        glm::ivec3 sensor_pos_block = get_coord_containing(pos);
        glm::ivec3 player_pos_block = get_coord_containing(pl->pos);

        glm::vec3 delta = glm::vec3(player_pos_block - sensor_pos_block);

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
            auto network = *cwire_man.get_instance_data(ce).network;

            comms_msg msg{
                ce,
                comms_msg_type_proximity_sensor_state,
                (*(proximity.is_detected)) ? 1.0f : 0.0f,
            };

            publish_msg(ship, network, msg);
        }
    }
}


void
tick_readers(ship_space *ship) {
    auto &reader_man = component_system_man.managers.reader_component_man;
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;

    for (auto i = 0u; i < reader_man.buffer.num; i++) {
        auto ce = reader_man.instance_pool.entity[i];

        auto const &cwire = cwire_man.get_instance_data(ce);
        auto const &net = ship->get_comms_network(*cwire.network);

        for (auto msg : net.read_buffer) {
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

extern GLuint modelspace_uv_shader;

void
draw_renderables(frame_data *frame)
{
    auto &render_man = component_system_man.managers.renderable_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;
    auto &display_man = component_system_man.managers.display_component_man;

    for (auto i = 0u; i < render_man.buffer.num; i++) {
        auto ce = render_man.instance_pool.entity[i];
        auto & mesh_name = render_man.instance_pool.mesh[i];
        auto & mesh = asset_man.get_mesh(mesh_name);

        if (!render_man.instance_pool.draw[i]) {
            continue;
        }

        auto params = frame->alloc_aligned<glm::mat4>(1);
        *(params.ptr) = *pos_man.get_instance_data(ce).mat;
        params.bind(1, frame);

        draw_mesh(mesh.hw);
    }

    // TODO: stop using this hack entirely.
    // Display meshes need to come with correct UVs.
    glUseProgram(modelspace_uv_shader);

    asset_man.bind_render_textures(0);

    for (auto i = 0u; i < display_man.buffer.num; i++) {
        auto ce = display_man.instance_pool.entity[i];
        auto & mesh_name = display_man.instance_pool.mesh[i];
        auto & mesh = asset_man.get_mesh(mesh_name);

        if (!render_man.get_instance_data(ce).draw) {
            continue;
        }

        auto params = frame->alloc_aligned<display_mesh_instance>(1);
        params.ptr->world_matrix = *pos_man.get_instance_data(ce).mat;
        params.ptr->material = i;
        params.bind(1, frame);

        draw_mesh(mesh.hw);
    }
}


void
draw_doors(frame_data *frame)
{
    auto &door_man = component_system_man.managers.door_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;

    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];
        auto & mesh_name = door_man.instance_pool.mesh[i];

        if (!mesh_name) {
            continue;
        }

        auto & mesh = asset_man.get_mesh(mesh_name);

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
