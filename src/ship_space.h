#pragma once

#include <glm/glm.hpp> /* ivec3 */
#include <set>
#include <unordered_map>
#include <array>

#include "block.h"
#include "common.h"
#include "component/component_manager.h"
#include "chunk.h"
#include "wiring/wiring.h"
#include "wiring/wiring_data.h"
#include <unordered_set>

extern void remove_ents_from_surface(glm::ivec3 b, int face);

struct ivec3_hash {
  size_t operator()(const glm::ivec3 &v) const {
      std::hash<int> h;
      size_t hh = h(v.x);
      hh = hh>>6 ^ hh<<2 ^ h(v.y);
      hh = hh>>6 ^ hh<<2 ^ h(v.z);
      return hh;
  }
};

struct zone_info {
    float gas_amount[int(gas::upper_bound)];
};

enum raycast_stopping_rule {
    enter_exit_framing = 1,
    cross_surface = 2,
};

struct ship_space {
    /* the min and max chunk co-ords ship_space has seen for each axis
     * this is for iteration (min_x..max_x) (inclusive)
     *
     * ship_space is sparse so even within this range
     * chunks may still be null
     */
    glm::ivec3 mins;
    glm::ivec3 maxs;

    std::unordered_map<glm::ivec3, chunk*, ivec3_hash> chunks;
    std::unordered_map<topo_info *, zone_info *> zones;

    // fixed pools of networks
    // trade-off of contiguous memory access and unused memory
    std::array<power_wiring_data, MAX_NETWORKS> power_networks {};
    std::array<comms_wiring_data, MAX_NETWORKS> comms_networks {};

    power_wiring_data & get_power_network(unsigned network) {
        return power_networks[network];
    }

    comms_wiring_data & get_comms_network(unsigned network) {
        return comms_networks[network];
    }

    /* create an empty ship_space */
    ship_space();

    /* returns a block or null
     * finds the block at the position (x,y,z) within
     * the whole ship_space
     * will move across chunks
     */
    block * get_block(glm::ivec3 block);

    topo_info * get_topo_info(glm::ivec3 block);

    /* returns the chunk containing the block denotated by (x, y, z)
     * or null
     */
    chunk * get_chunk_containing(glm::ivec3 block);

    /* returns the chunk corresponding to the chunk coordinates (x, y, z)
     * note this is NOT using block coordinates
     */
    chunk * get_chunk(glm::ivec3 chunk);

    /* returns a pointer to a new ship space
     * this ship space will have 2 x 2 rooms and will be 1 room tall
     * each room will have a floor and 4 walls of framing 
     * each room will have some doors on all 4 walls
     * there will be a floor of surfaces
     * and will otherwise be empty
     *
     * returns 0 on error
     */
    static ship_space * mock_ship_space();

    bool raycast_block(glm::vec3 o, glm::vec3 d, float max_reach_distance, raycast_stopping_rule stopping_rule, raycast_info_block *rc);

    /* ensure that the specified block_{x,y,z} can be fetched with a get_block
     *
     * this will instantiate a new containing chunk if necessary
     */
    block * ensure_block(glm::ivec3 block);

    /* ensure that the specified chunk exists
     *
     * this will instantiate a new chunk if necessary
     */
    chunk * ensure_chunk(glm::ivec3 chunk);

    zone_info *get_zone_info(topo_info *t);
    void insert_zone(topo_info *t, zone_info *z);

    /* topo info for open vacuum, so we know what pressure to force to zero */
    topo_info outside_topo_info;
    void rebuild_topology();
    void update_topology_for_remove_surface(glm::ivec3 a, glm::ivec3 b);
    void update_topology_for_add_surface(glm::ivec3 a, glm::ivec3 b, int face);

    int num_full_rebuilds;      /* number of full rebuilds (pretty slow) performed */
    int num_fast_unifys;        /* number of incremental unify operations performed */
    int num_fast_nosplits;      /* number of rebuilds avoided because we proved them spurious */
    int num_false_splits;       /* number of useless rebuilds taken */

    bool validate();

    void set_surface(glm::ivec3 a, glm::ivec3 b, surface_index index,
        surface_type st);

    bool find_next_block(glm::ivec3 start, glm::ivec3 dir, unsigned limit, glm::ivec3 *found);

    /* returns the neighbor of a block along a given surface's normal
     * finds the block at the position (x,y,z) within
     * the whole ship_space
     * will move across chunks
     * will call ensure_block if needed
     */
    block *get_block_neighbor(glm::ivec3 block, enum surface_index si);

    void remove_block(glm::ivec3 p);

    /* removes a cuboid defined by min and max extents
     * set border surfaces where block present to {type}
     */
    void cut_out_cuboid(glm::ivec3 mins, glm::ivec3 maxs, surface_type type);

    /* convert a topo ptr to the corresponding location. not expected to be fast. */
    bool topo_to_pos(topo_info *t, glm::ivec3* out);

    glm::ivec3 get_chunk_coord_containing(glm::ivec3 block);
};

/* helper */
topo_info *
topo_find(topo_info *p);

