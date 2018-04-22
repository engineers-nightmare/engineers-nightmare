#include <glm/gtc/random.hpp>
#include <memory>

#include "component_system_manager.h"
#include "../asset_manager.h"
#include "../particle.h"
#include "../mesh.h"
#include "../entity_utils.h"
#include "../common.h"

extern asset_manager asset_man;
extern component_system_manager component_system_man;
extern frame_info frame_info;

extern particle_manager *particle_man;

static bool
filter_matches_message(comms_msg const &msg, wire_filter_ptr const &filter) {
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;
    auto sender = cwire_man.get_instance_data(msg.originator);

    /* If we have no filter defined, we match everything. */
    if (!filter.wrapped)
        return true;

    /* We have a filter. Sender must have a filter, and it must match. */
    return *sender.label && !strcmp(*sender.label, filter.wrapped->c_str());
}

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
        for (auto msg : net.read_buffer) {
            if (!filter_matches_message(msg, gas_man.instance_pool.filter[i]) || msg.type != msg_type::switch_transition) {
                continue;
            }

            auto data = clamp(msg.data, 0.0f, 1.0f);
            gas_man.instance_pool.enabled[i] = data > 0;
            *power.required_power = data > 0 ? *power.max_required_power : 0.0f;
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
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < door_man.buffer.num; i++) {
        auto ce = door_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto cwire = cwire_man.get_instance_data(ce);

        /* it's a power door, it's not going /anywhere/ without power */
        if (!*power.powered) {
            continue;
        }

        /* todo: origin discrimination */
        /* todo: what if many transitions requested? */
        auto const &net = ship->get_comms_network(*cwire.network);

        for (auto msg : net.read_buffer) {
            if (msg.type == msg_type::switch_transition) {
                continue;
            }

            door_man.instance_pool.desired_pos[i] = clamp(msg.data, 0.0f, 1.0f);
        }

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
    auto &cwire_man = component_system_man.managers.wire_comms_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < light_man.buffer.num; i++) {
        auto ce = light_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto light = light_man.get_instance_data(ce);

        if (!*power.powered) {
            return;
        }

        auto const &cwire = cwire_man.get_instance_data(ce);
        auto const &net = ship->get_comms_network(*cwire.network);

        for (auto msg : net.read_buffer) {
            if (!filter_matches_message(msg, *light.filter) || msg.type != msg_type::switch_transition)
                continue;

            *(light.requested_intensity) = clamp(msg.data, 0.0f, 1.0f);

            auto old_intensity = *(light.intensity);
            auto new_intensity = *power.powered ? *(light.requested_intensity) : 0.0f;

            if (old_intensity != new_intensity) {
                *(light.intensity) = new_intensity;
                *(power.required_power) = *(light.requested_intensity) * *(power.max_required_power);
            }
        }
    }
}


void
tick_rotator_components(ship_space *ship) {
    auto &rot_man = component_system_man.managers.rotator_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;
    auto &par_man = component_system_man.managers.parent_component_man;
//    auto &cwire_man = component_system_man.managers.wire_comms_component_man;
    auto &power_man = component_system_man.managers.power_component_man;

    for (auto i = 0u; i < rot_man.buffer.num; i++) {
        auto ce = rot_man.instance_pool.entity[i];

        auto power = power_man.get_instance_data(ce);
        auto rot = rot_man.get_instance_data(ce);
        auto pos = pos_man.get_instance_data(ce);

        if (!*power.powered) {
            return;
        }

//        auto const &cwire = cwire_man.get_instance_data(ce);
//        auto const &net = ship->get_comms_network(*cwire.network);
//
//        for (auto msg : net.read_buffer) {
//            auto filter = rot.filter->c_str();
//            auto sender = cwire_man.get_instance_data(msg.originator);
//
//            if ((*sender.label == nullptr || filter == nullptr) || strcmp(*sender.label, filter) != 0 || msg.type == msg_type::switch_transition) {
//                continue;
//            }
//
//            *rot.rot_cur_speed = *rot.rot_cur_speed ? 0 : *rot.rot_speed;
//        }
        *rot.rot_cur_speed = *rot.rot_cur_speed ? 0 : *rot.rot_speed;

        glm::mat4 pos_mat;
        if (par_man.exists(ce)) {
            auto par = par_man.get_instance_data(ce);
            pos_mat = *par.local_mat;
        }
        else {
            pos_mat = *pos.mat;
        }
        if (*rot.rot_cur_speed) {
            pos_mat = glm::rotate(pos_mat, (float)*rot.rot_dir * *rot.rot_cur_speed * (float)frame_info.dt, *rot.rot_axis);
            pos_mat[3] = glm::vec4(*rot.rot_offset, 1.0f);
        }

        set_entity_matrix(ce, pos_mat);
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
        auto desc = msg_type::pressure_sensor_1;
        if (which_sensor == 2) {
            desc = msg_type::pressure_sensor_2;
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
            if (sensor_1 == FLT_MAX && msg.type == msg_type::pressure_sensor_1) {
                sensor_1 = msg.data;
            }
            if (sensor_2 == FLT_MAX && msg.type == msg_type::pressure_sensor_2) {
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
            msg_type::sensor_comparison,
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
                msg_type::proximity_sensor,
                (*(proximity.is_detected)) ? 1.0f : 0.0f,
            };

            publish_msg(ship, network, msg);
        }
    }
}

void
build_absolute_transforms() {
    auto &parent_man = component_system_man.managers.parent_component_man;
    auto &pos_man = component_system_man.managers.position_component_man;

    for (auto i = 0u; i < parent_man.buffer.num; i++) {
        auto ce = parent_man.instance_pool.entity[i];
        auto parent = parent_man.instance_pool.parent[i];
        *pos_man.get_instance_data(ce).mat = *pos_man.get_instance_data(parent).mat * parent_man.instance_pool.local_mat[i];
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
