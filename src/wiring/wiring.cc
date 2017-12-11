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

    for (auto &net : ship->power_networks) {
        net = {};
    }

    /* walk power consumers */
    /* invariant: at most /one/ power wire is attached. */
    for (auto i = 0u; i < power_man.buffer.num; i++) {
        auto &net = ship->get_power_network(power_man.instance_pool.network[i]);

        net.num_consumers++;
        net.peak_draw += power_man.instance_pool.max_required_power[i];
        net.total_draw += power_man.instance_pool.required_power[i];
    }

    /* walk power producers */
    /* invariant: at most /one/ power wire is attached. */
    for (auto i = 0u; i < power_provider_man.buffer.num; i++) {
        auto &net = ship->get_power_network(power_provider_man.instance_pool.network[i]);
        // TODO: model provided vs max_provided here.
        net.total_power += power_provider_man.instance_pool.max_provided[i];
        net.num_providers++;
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
    for (auto & net : ship->comms_networks) {
        std::swap(net.read_buffer, net.write_buffer);
        net.write_buffer.clear();
    }
}

/* The common case for publishing comms_msg: broadcast to all connected wires. */
void publish_msg(ship_space *ship, unsigned network, comms_msg msg) {
    ship->comms_networks[network].write_buffer.push_back(msg);
}
