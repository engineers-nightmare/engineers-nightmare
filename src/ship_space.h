#pragma once

#include <glm/glm.hpp> /* ivec3 */
#include <set>
#include <unordered_map>

#include "block.h"
#include "component/component_manager.h"
#include "chunk.h"
#include "wiring/wiring.h"
#include "wiring/wiring_data.h"
#include <unordered_set>

struct ivec3_hash {
  size_t operator()(const glm::ivec3 &v) const {
      std::hash<int> h;
      size_t hh = h(v.x);
      hh = hh>>6 ^ hh<<2 ^ h(v.y);
      hh = hh>>6 ^ hh<<2 ^ h(v.z);
      return hh;
  }
};

struct raycast_info {
    bool hit;
    bool inside;
    glm::ivec3 bl;          /* the block we hit */
    glm::ivec3 n;           /* the face normal we hit */
    glm::ivec3 p;           /* the block along the normal */
    struct block *block;
};

struct zone_info {
    float air_amount;

    zone_info(float air_amount) : air_amount(air_amount) {}
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

    std::vector<wire_attachment> wire_attachments[num_wire_types];
    std::vector<wire_segment> wire_segments[num_wire_types];

    /* for rendering currently edited wire */
    unsigned active_wire[num_wire_types][2];

    std::unordered_map<c_entity, std::unordered_set<unsigned>> entity_to_attach_lookups[num_wire_types];

    std::unordered_map<unsigned, power_wiring_data> power_wires;
    std::unordered_map<unsigned, comms_wiring_data> comms_wires;

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
    static ship_space * mock_ship_space(void);

    void raycast(glm::vec3 o, glm::vec3 d, raycast_info *rc);

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

    /* serialization methods for chunks
     */
    std::vector<unsigned char> * serialize_chunk(glm::ivec3 ch);
    bool unserialize_chunk(glm::ivec3 ch, unsigned char *data, size_t len);

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

    void set_block(glm::ivec3 block, block_type type);
};

/* helper */
topo_info *
topo_find(topo_info *p);


static inline int
normal_to_surface_index(raycast_info const *rc)
{
    if (rc->n.x == 1) return 0;
    if (rc->n.x == -1) return 1;
    if (rc->n.y == 1) return 2;
    if (rc->n.y == -1) return 3;
    if (rc->n.z == 1) return 4;
    if (rc->n.z == -1) return 5;

    return 0;   /* unreachable */
}


static inline glm::ivec3
surface_index_to_normal(int index)
{
    glm::ivec3 n(0, 0, 0);

    switch (index) {
        case 0: n.x = 1; break;
        case 1: n.x = -1; break;
        case 2: n.y = 1; break;
        case 3: n.y = -1; break;
        case 4: n.z = 1; break;
        case 5: n.z = -1; break;
    }

    return n;
}
