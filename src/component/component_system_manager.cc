#include "component_system_manager.h"

gas_production_component_manager gas_man;
light_component_manager light_man;
physics_component_manager physics_man;
relative_position_component_manager pos_man;
power_component_manager power_man;
power_provider_component_manager power_provider_man;
renderable_component_manager render_man;
surface_attachment_component_manager surface_man;
switch_component_manager switch_man;
switchable_component_manager switchable_man;
type_component_manager type_man;
updateable_component_manager updateable_man;


extern void mark_lightfield_update(int x, int y, int z);


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
        topo_info *t = topo_find(ship->get_topo_info(pos.x, pos.y, pos.z));
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

        auto & powered = power_man.powered(ce) = false;

        if (ship->entity_to_power_attach_lookup.find(ce) == ship->entity_to_power_attach_lookup.end()) {
            continue;
        }

        std::set<unsigned> visited_wires;
        auto const & attaches = ship->entity_to_power_attach_lookup[ce];
        for (auto const & sea : attaches) {
            auto const & attach = ship->power_attachments[sea];
            auto wire_index = attach_topo_find(ship, wire_type_power, attach.parent);
            if (visited_wires.find(wire_index) != visited_wires.end()) {
                continue;
            }

            auto const & wire = ship->power_wires[wire_index];

            visited_wires.insert(wire_index);
            /* todo: this needs to somehow handle multiple wires */
            powered |= wire.total_power >= wire.total_draw;
        }


void
tick_updateables(ship_space * ship) {
    for (auto i = 0u; i < updateable_man.buffer.num; i++) {
        auto const & entity = updateable_man.instance_pool.entity[i];

        /* _something_ updated last frame */
        if (updateable_man.instance_pool.updated[i]) {
            /* was it lights? */
            if (light_man.exists(entity)) {
                /* could have been. mark it */
                auto pos = pos_man.position(entity);
                auto block_pos = get_coord_containing(pos);
                mark_lightfield_update(block_pos.x, block_pos.y, block_pos.z);
            }
        }

        updateable_man.instance_pool.updated[i] = false;
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
