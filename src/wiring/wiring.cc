#include <unordered_set>

#include "../asset_manager.h"
#include "../common.h"
#include "wiring.h"
#include "../mesh.h"
#include "../ship_space.h"
#include "../component/component_system_manager.h"

extern asset_manager asset_man;
extern component_system_manager component_system_man;

extern GLuint simple_shader;
extern GLuint unlit_instanced_shader;
extern GLuint lit_instanced_shader;


unsigned
attach_topo_find(ship_space *ship, wire_type type, unsigned p) {
    // HACK: everything's connected in one big network.
    return 0;
}


/* calculates power data for each power run
 * assumes a rebuilt attach topo
 */
void
calculate_power_wires(ship_space *ship) {
    auto &power_man = component_system_man.managers.power_component_man;
    auto &power_provider_man = component_system_man.managers.power_provider_component_man;

    ship->power_wires.clear();
    const auto type = wire_type_power;

    /* note: power_wires operator[] default-constructs the value if not present. */

    /* walk power consumers */
    /* invariant: at most /one/ power wire is attached. */
    for (auto i = 0u; i < power_man.buffer.num; i++) {
        auto ce = power_man.instance_pool.entity[i];
        auto attaches = ship->entity_to_attach_lookups[type].find(ce);
        if (attaches != ship->entity_to_attach_lookups[type].end()) {
            for (auto attach : attaches->second) {
                auto wire = attach_topo_find(ship, type, attach);
                auto & power_data = ship->power_wires[wire];

                power_data.num_consumers++;
                power_data.peak_draw += power_man.instance_pool.max_required_power[i];
                power_data.total_draw += power_man.instance_pool.required_power[i];
            }
        }
    }

    /* walk power producers */
    /* invariant: at most /one/ power wire is attached. */
    for (auto i = 0u; i < power_provider_man.buffer.num; i++) {
        auto ce = power_provider_man.instance_pool.entity[i];
        auto attaches = ship->entity_to_attach_lookups[type].find(ce);
        if (attaches != ship->entity_to_attach_lookups[type].end()) {
            for (auto attach : attaches->second) {
                auto wire = attach_topo_find(ship, type, attach);
                auto & power_data = ship->power_wires[wire];

                power_data.total_power += power_provider_man.instance_pool.provided[i];
                power_data.num_providers++;
            }
        }
    }
}

/* Phase boundary: this frame's written messages become the next frame's messages to read.
 * start off the next frame's written messages as empty.
 *
 * This allows any tick code to read or write messages however it likes, but never step on each
 * other or introduce strange order-based inconsistencies.
 */
void
propagate_comms_wires(ship_space *ship) {
    for (auto & w : ship->comms_wires) {
        std::swap(w.second.read_buffer, w.second.write_buffer);
        w.second.write_buffer.clear();
    }
}


/* The common case for publishing comms_msg: broadcast to all connected wires. */
void
publish_msg(ship_space *ship, c_entity ce, comms_msg msg)
{
    auto & comms_attaches = ship->entity_to_attach_lookups[wire_type_comms];
    auto attaches = comms_attaches.find(ce);
    if (attaches == comms_attaches.end()) {
        return;
    }

    std::unordered_set<unsigned> visited_wires;
    for (auto sea : attaches->second) {
        auto wire_index = attach_topo_find(ship, wire_type_comms, sea);
        if (visited_wires.find(wire_index) != visited_wires.end()) {
            continue;
        }

        visited_wires.insert(wire_index);
        ship->comms_wires[wire_index].write_buffer.push_back(msg);
    }
}


void
remove_attaches_for_entity(ship_space *ship, c_entity ce)
{
    for (auto _type = 0; _type < num_wire_types; _type++) {
        auto type = (wire_type)_type;
        auto & entity_to_attach_lookup = ship->entity_to_attach_lookups[type];
        auto & wire_attachments = ship->wire_attachments[type];

        auto entity_attaches = entity_to_attach_lookup.find(ce);
        if (entity_attaches != entity_to_attach_lookup.end()) {
            auto const & set = entity_attaches->second;
            auto attaches = std::vector<unsigned>(set.begin(), set.end());
            std::sort(attaches.begin(), attaches.end());

            /* Remove relevant attaches from wire_attachments
            * relevant is an attach that isn't occupying a position
            * will get popped off as a result of moving before removing
            */
            auto swap_index = (unsigned)wire_attachments.size() - 1;
            for (auto att_index = (unsigned)attaches.size() - 1; att_index != invalid_attach; --att_index) {

                auto from_attach = wire_attachments[swap_index];
                auto rem = attaches[att_index];
                if (swap_index > rem) {
                    wire_attachments[rem] = from_attach;
                    wire_attachments.pop_back();
                    --swap_index;
                }
                else if (swap_index == rem) {
                    wire_attachments.pop_back();
                    --swap_index;
                }
            }

            /* remove attaches assigned to entity from ship lookup */
            entity_to_attach_lookup.erase(ce);
        }
    }
}
