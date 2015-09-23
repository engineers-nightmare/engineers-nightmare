#include "component_system_manager.h"

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
switchable_component_manager switchable_man;
type_component_manager type_man;


extern void mark_lightfield_update(glm::ivec3 center);


/* I have no clue how we're going to actually handle these */
const char * comms_msg_type_switch_state = "switch_state";
const char * comms_msg_type_pressure_sensor_state = "pressure_sensor_state";


void
tick_gas_producers(ship_space * ship)
{
    for (auto i = 0u; i < gas_man.buffer.num; i++) {
        auto ce = gas_man.instance_pool.entity[i];

        /* gas producers require: power, position */
        assert(switchable_man.exists(ce) || !"gas producer must be switchable");

        auto should_produce = switchable_man.enabled(ce) && power_man.powered(ce);
        if (!should_produce) {
            return;
        }

        auto pos = get_coord_containing(pos_man.position(ce));

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
        }
    }
}


void
tick_power_consumers(ship_space * ship) {
    for (auto i = 0u; i < power_man.buffer.num; i++) {
        auto ce = power_man.instance_pool.entity[i];

        auto & powered = power_man.powered(ce);
        auto old_powered = powered;
        powered = false;

        auto & power_attaches = ship->entity_to_attach_lookups[wire_type_power];
        auto attaches = power_attaches.find(ce);
        if (attaches == power_attaches.end()) {
            continue;
        }

        std::unordered_set<unsigned> visited_wires;
        for (auto const & sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, wire_type_power, sea);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto const & wire = ship->power_wires[wire_index];

            visited_wires.insert(wire_index);
            /* todo: this needs to somehow handle multiple wires */
            powered |= wire.total_power >= wire.total_draw;
        }

        if (powered != old_powered) {
            /* if a light changed power state, do the required update now */
            if (light_man.exists(ce)) {
                auto pos = pos_man.position(ce);
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
        auto type = wire_type_comms;

        /* all lights currently require: switchable, position */
        assert(switchable_man.exists(ce) || !"lights must be switchable");
        assert(pos_man.exists(ce) || !"lights must have a position");

        auto & comms_attaches = ship->entity_to_attach_lookups[type];
        auto attaches = comms_attaches.find(ce);
        if (attaches == comms_attaches.end()) {
            continue;
        }

        std::unordered_set<unsigned> visited_wires;
        for (auto const & sea : attaches->second) {
            auto wire_index = attach_topo_find(ship, type, sea);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto const & wire = ship->comms_wires[wire_index];

            visited_wires.insert(wire_index);

            /* now that we have the wire, see if it has any msgs for us */
            /* todo: origin discrimination */
            for (auto msg : wire.read_buffer) {
                if (msg.desc == comms_msg_type_switch_state ||
                    msg.desc == comms_msg_type_pressure_sensor_state) {

                    auto data = clamp(msg.data, 0.f, 1.f);
                    light_man.intensity(ce) = data;
                    switchable_man.enabled(ce) = data > 0;

                    auto pos = pos_man.position(ce);
                    auto block_pos = get_coord_containing(pos);
                    mark_lightfield_update(block_pos);
                }
            }
        }
    }
}

void tick_pressure_sensors(ship_space* ship) {
    for (auto i = 0u; i < pressure_man.buffer.num; i++) {
        auto ce = pressure_man.instance_pool.entity[i];
        auto type = wire_type_comms;

        /* all pressure sensors currently require: position */
        assert(pos_man.exists(ce) || !"pressure sensors must have a position");

        auto pos = pos_man.position(ce);

        glm::ivec3 pos_block = get_coord_containing(pos);

        topo_info *t = topo_find(ship->get_topo_info(pos_block));
        topo_info *outside = topo_find(&ship->outside_topo_info);
        zone_info *z = ship->get_zone_info(t);
        float pressure = z ? (z->air_amount / t->size) : 0.0f;

        auto wire_type = wire_type_comms;
        auto & comms_attaches = ship->entity_to_attach_lookups[wire_type];

        if (comms_attaches.find(ce) == comms_attaches.end()) {
            return;
        }

        std::unordered_set<unsigned> visited_wires;
        auto const & attaches = comms_attaches[ce];
        for (auto const & sea : attaches) {
            auto const & attach = ship->wire_attachments[wire_type][sea];
            auto wire_index = attach_topo_find(ship, wire_type, attach.parent);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            visited_wires.insert(wire_index);

            comms_msg msg;
            msg.originator = ce;
            msg.desc = comms_msg_type_pressure_sensor_state;
            msg.data = pressure;
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
        auto & mat = pos_man.mat(ce);

        auto entity_matrix = frame->alloc_aligned<glm::mat4>(1);
        *entity_matrix.ptr = mat;
        entity_matrix.bind(1, frame);

        draw_mesh(&mesh);
    }
}
