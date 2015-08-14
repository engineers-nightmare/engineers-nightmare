#pragma once

#include "block.h"
#include "chunk.h"
#include "wiring.h"

#include <glm/glm.hpp> /* ivec3 */
#include <unordered_map>

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
    int x, y, z;            /* the block we hit */
    int nx, ny, nz;         /* the face normal we hit */
    int px, py, pz;         /* the block along the normal */
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
    int min_x, min_y, min_z;
    int max_x, max_y, max_z;

    /* internal method which updated {min,max}_{x,y,z}
     * if the {x,y,z}_seen values are lower/higher
     */
    void _maintain_bounds(int x_seen, int y_seen, int z_seen);

    std::unordered_map<glm::ivec3, chunk*, ivec3_hash> chunks;
    std::unordered_map<topo_info *, zone_info *> zones;

    std::vector<wire_attachment> wire_attachments;
    std::vector<wire_segment> wire_segments;

    /* create an empty ship_space */
    ship_space();

    /* returns a block or null
     * finds the block at the position (x,y,z) within
     * the whole ship_space
     * will move across chunks
     */
    block * get_block(int block_x, int block_y, int block_z);

    topo_info * get_topo_info(int block_x, int block_y, int block_z);

    /* returns the chunk containing the block denotated by (x, y, z)
     * or null
     */
    chunk * get_chunk_containing(int block_x, int block_y, int block_z);

    /* returns the chunk corresponding to the chunk coordinates (x, y, z)
     * note this is NOT using block coordinates
     */
    chunk * get_chunk(int chunk_x, int chunk_y, int chunk_z);

    /* returns a pointer to a new ship space
     * this ship space will have 2 x 2 rooms and will be 1 room tall
     * each room will have a floor and 4 walls of scaffolding
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
     *
     * this will not instantiate or modify any other chunks
     */
    block * ensure_block(int block_x, int block_y, int block_z);

    /* ensure that the specified chunk exists
     *
     * this will instantiate a new chunk if necessary
     *
     * this will not instantiate or modify any other chunks
     */
    chunk * ensure_chunk(int chunk_x, int chunk_y, int chunk_z);

    /* serialization methods for chunks
     */
    std::vector<unsigned char> * serialize_chunk(int chunk_x, int chunk_y, int chunk_z);
    bool unserialize_chunk(int chunk_x, int chunk_y, int chunk_z, unsigned char *data, size_t len);

    zone_info *get_zone_info(topo_info *t);
    void insert_zone(topo_info *t, zone_info *z);

    /* topo info for open vacuum, so we know what pressure to force to zero */
    topo_info outside_topo_info;
    void rebuild_topology();
    void update_topology_for_remove_surface(int x, int y, int z, int px, int py, int pz);
    void update_topology_for_add_surface(int x, int y, int z, int px, int py, int pz, int face);

    int num_full_rebuilds;      /* number of full rebuilds (pretty slow) performed */
    int num_fast_unifys;        /* number of incremental unify operations performed */
    int num_fast_nosplits;      /* number of rebuilds avoided because we proved them spurious */
    int num_false_splits;       /* number of useless rebuilds taken */

    bool validate();
};

/* helper */
topo_info *
topo_find(topo_info *p);


static inline int
normal_to_surface_index(raycast_info const *rc)
{
    if (rc->nx == 1) return 0;
    if (rc->nx == -1) return 1;
    if (rc->ny == 1) return 2;
    if (rc->ny == -1) return 3;
    if (rc->nz == 1) return 4;
    if (rc->nz == -1) return 5;

    return 0;   /* unreachable */
}


static inline void
surface_index_to_normal(int index, int *nx, int *ny, int *nz)
{
    *nx = *ny = *nz = 0;

    switch (index) {
        case 0: *nx = 1; break;
        case 1: *nx = -1; break;
        case 2: *ny = 1; break;
        case 3: *ny = -1; break;
        case 4: *nz = 1; break;
        case 5: *nz = -1; break;
    }
}
