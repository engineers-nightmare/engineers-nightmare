#include "ship_space.h"
#include <assert.h>
#include <math.h>
#include <algorithm>


#define MAX_WIRE_INSTANCES 64 * 1024


/* create an empty ship_space */
ship_space::ship_space(void)
    : min_x(0), min_y(0), min_z(0), max_x(0), max_y(0), max_z(0),
      num_full_rebuilds(0), num_fast_unifys(0), num_fast_nosplits(0), num_false_splits(0)
{
    /* start rather large */
    power_wires.reserve(MAX_WIRE_INSTANCES);
}


static void
split_coord(int p, int *out_block, int *out_chunk)
{
    /* NOTE: There are a number of attractive-looking symmetries which are
     * just plain wrong. */
    int block, chunk;

    if (p < 0) {
        /* negative space is not a mirror of positive:
         * chunk -1 spans blocks -8..-1;
         * chunk -2 spans blocks -16..-9 */
        chunk = (p - CHUNK_SIZE + 1) / CHUNK_SIZE;
    } else {
        /* positive halfspace has no rocket science. */
        chunk = p / CHUNK_SIZE;
    }

    /* the within-chunk offset is just the difference between the minimum block
     * in the chunk and the requested one, regardless of which halfspace we're in. */
    block = p - CHUNK_SIZE * chunk;

    /* write the outputs which were requested */
    if (out_block)
        *out_block = block;
    if (out_chunk)
        *out_chunk = chunk;
}


/* returns a block or null
 * finds the block at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 */
block *
ship_space::get_block(int block_x, int block_y, int block_z)
{
    /* Within Block coordinates */
    int wb_x, wb_y, wb_z;
    int chunk_x, chunk_y, chunk_z;

    split_coord(block_x, &wb_x, &chunk_x);
    split_coord(block_y, &wb_y, &chunk_y);
    split_coord(block_z, &wb_z, &chunk_z);

    chunk *c = this->get_chunk(chunk_x, chunk_y, chunk_z);

    if( ! c ){
        return 0;
    }

    return c->blocks.get(wb_x, wb_y, wb_z);
}

/* returns a topo_info or null
 * finds the topo_info at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 */
topo_info *
ship_space::get_topo_info(int block_x, int block_y, int block_z)
{
    /* Within Block coordinates */
    int wb_x, wb_y, wb_z;
    int chunk_x, chunk_y, chunk_z;

    split_coord(block_x, &wb_x, &chunk_x);
    split_coord(block_y, &wb_y, &chunk_y);
    split_coord(block_z, &wb_z, &chunk_z);

    chunk *c = this->get_chunk(chunk_x, chunk_y, chunk_z);

    if (!c) {
        return &this->outside_topo_info;
    }

    return c->topo.get(wb_x, wb_y, wb_z);
}

zone_info *
ship_space::get_zone_info(topo_info *t)
{
    auto it = zones.find(t);
    if (it == zones.end())
        return nullptr;

    return it->second;
}

/* returns the chunk containing the block denotated by (x, y, z)
 * or null
 */
chunk *
ship_space::get_chunk_containing(int block_x, int block_y, int block_z)
{
    int chunk_x, chunk_y, chunk_z;

    split_coord(block_x, nullptr, &chunk_x);
    split_coord(block_y, nullptr, &chunk_y);
    split_coord(block_z, nullptr, &chunk_z);

    return this->get_chunk(chunk_x, chunk_y, chunk_z);
}

/* returns the chunk corresponding to the chunk coordinates (x, y, z)
 * note this is NOT using block coordinates
 */
chunk *
ship_space::get_chunk(int chunk_x, int chunk_y, int chunk_z)
{
    glm::ivec3 v(chunk_x, chunk_y, chunk_z);

    auto it = this->chunks.find(v);
    if( it != this->chunks.end() ){
        return it->second;
    }

    return nullptr;
}



static float
max_along_axis(float o, float d)
{
    if (d > 0) {
        return fabsf((ceilf(o) - o)/d);
    }
    else {
        return fabsf((floorf(o) - o)/d);
    }
}

/* max reach, counted in edge-crossings. for spherical reach, the results need to be
 * further pruned -- this allows ~2 blocks in the worst case diagonals, and 6 in the
 * best cases, where only one axis is traversed.
 */
#define MAX_PLAYER_REACH 6


void
ship_space::raycast(glm::vec3 o, glm::vec3 d, raycast_info *rc)
{
    /* implementation of the algorithm described in
     * http://www.cse.yorku.ca/~amana/research/grid.pdf
     */

    assert(rc);
    rc->hit = false;

    /* if less than 0 we need to subtract one
     * as float truncation will bias
     * towards 0
     */
    int x = (int)(o.x < 0 ? o.x - 1: o.x);
    int y = (int)(o.y < 0 ? o.y - 1: o.y);
    int z = (int)(o.z < 0 ? o.z - 1: o.z);

    int nx = 0;
    int ny = 0;
    int nz = 0;

    block *bl = nullptr;

    bl = this->get_block(x,y,z);
    rc->inside = bl ? bl->type != block_empty : 0;

    int stepX = d.x > 0 ? 1 : -1;
    int stepY = d.y > 0 ? 1 : -1;
    int stepZ = d.z > 0 ? 1 : -1;

    float tDeltaX = fabsf(1/d.x);
    float tDeltaY = fabsf(1/d.y);
    float tDeltaZ = fabsf(1/d.z);

    float tMaxX = max_along_axis(o.x, d.x);
    float tMaxY = max_along_axis(o.y, d.y);
    float tMaxZ = max_along_axis(o.z, d.z);

    for (int i = 0; i < MAX_PLAYER_REACH; ++i) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                tMaxX += tDeltaX;
                nx = -stepX;
                ny = 0;
                nz = 0;
            }
            else {
                z += stepZ;
                tMaxZ += tDeltaZ;
                nx = 0;
                ny = 0;
                nz = -stepZ;
            }
        }
        else {
            if (tMaxY < tMaxZ) {
                y += stepY;
                tMaxY += tDeltaY;
                nx = 0;
                ny = -stepY;
                nz = 0;
            }
            else {
                z += stepZ;
                tMaxZ += tDeltaZ;
                nx = 0;
                ny = 0;
                nz = -stepZ;
            }
        }

        bl = this->get_block(x, y, z);
        if (!bl && !rc->inside){
            /* if there is no block then we are outside the grid
             * we still want to keep stepping until we either
             * hit a block within the grid or exceed our maximum
             * reach
             */
            continue;
        }

        if (rc->inside ^ (bl && bl->type != block_empty)) {
            rc->hit = true;
            rc->x = x;
            rc->y = y;
            rc->z = z;
            rc->block = bl;
            rc->nx = nx;
            rc->ny = ny;
            rc->nz = nz;
            rc->px = x + nx;
            rc->py = y + ny;
            rc->pz = z + nz;
            return;
        }
    }
}

/* ensure that the specified block_{x,y,z} can be fetched with a get_block
 *
 * this will instantiate a new containing chunk if necessary
 *
 * this will not instantiate or modify any other chunks
 */
block *
ship_space::ensure_block(int block_x, int block_y, int block_z)
{
    int chunk_x, chunk_y, chunk_z;

    split_coord(block_x, nullptr, &chunk_x);
    split_coord(block_y, nullptr, &chunk_y);
    split_coord(block_z, nullptr, &chunk_z);

    /* guarantee we have the size we need */
    this->ensure_chunk(chunk_x, chunk_y, chunk_z);

    return get_block(block_x, block_y, block_z);
}

/* ensure that the specified chunk exists
 *
 * this will instantiate a new chunk if necessary
 *
 * this will not instantiate or modify any other chunks
 */
chunk *
ship_space::ensure_chunk(int chunk_x, int chunk_y, int chunk_z)
{
    glm::ivec3 v(chunk_x, chunk_y, chunk_z);
    /* automatically creates the entry if not present */
    auto &ch = this->chunks[v];
    if (!ch) {
        ch = new chunk();
        this->_maintain_bounds(chunk_x, chunk_y, chunk_z);

        /* All the topo nodes in the new chunk should be attached
         * to the outside node.
         */
        for (auto k = 0; k < CHUNK_SIZE; k++) {
            for (auto j = 0; j < CHUNK_SIZE; j++) {
                for (auto i = 0; i < CHUNK_SIZE; i++) {
                    topo_info *t = ch->topo.get(i, j, k);
                    t->p = &this->outside_topo_info;
                }
            }
        }

        /* Adjust the size of the outside chunk. This is currently not
         * used for anything, but the consistency is nice and the cost is negligible.
         */
        this->outside_topo_info.size += CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    }

    return ch;
}

/* internal method which updated {min,max}_{x,y,z}
 * if the {x,y,z}_seen values are lower/higher
 */
void
ship_space::_maintain_bounds(int x_seen, int y_seen, int z_seen)
{
    this->min_x = std::min(min_x, x_seen);
    this->min_y = std::min(min_y, y_seen);
    this->min_z = std::min(min_z, z_seen);

    this->max_x = std::max(max_x, x_seen);
    this->max_y = std::max(max_y, y_seen);
    this->max_z = std::max(max_z, z_seen);
}

topo_info *
topo_find(topo_info *p)
{
    /* compress path */
    if (p->p != p) {
        p->p = topo_find(p->p);
    }

    return p->p;
}

/* helper to unify subtrees */
static topo_info *
topo_unite(topo_info *from, topo_info *to)
{
    from = topo_find(from);
    to = topo_find(to);

    /* already in same subtree? */
    if (from == to) return from;

    if (from->rank < to->rank) {
        from->p = to;
        return to;
    } else if (from->rank > to->rank) {
        to->p = from;
        return from;
    } else {
        /* merging two rank-r subtrees produces a rank-r+1 subtree. */
        to->p = from;
        from->rank++;
        return from;
    }
}

/* inserts a zone_info into the zone map. z may be
 * deleted, if there is an existing zone to merge into. */
void
ship_space::insert_zone(topo_info *t, zone_info *z)
{
    if (t == &outside_topo_info) {
        /* there is no point in combining with the outside. */
        delete z;
        return;
    }

    zone_info *existing_z = get_zone_info(t);
    if (existing_z) {
        /* merge case; mix in this zone, and then we'll delete it. */
        existing_z->air_amount += z->air_amount;
        delete z;
    }
    else {
        /* no zone here yet. this one will do fine! */
        zones[t] = z;
    }
}

void
ship_space::update_topology_for_remove_surface(int x, int y, int z, int px, int py, int pz)
{
    topo_info *t = topo_find(get_topo_info(x, y, z));
    topo_info *u = topo_find(get_topo_info(px, py, pz));

    num_fast_unifys++;

    if (t == u) {
        /* we're not really unifying */
        return;
    }

    zone_info *z1 = get_zone_info(t);
    zone_info *z2 = get_zone_info(u);

    /* remove the existing zones */
    if (z1) { zones.erase(zones.find(t)); }
    if (z2) { zones.erase(zones.find(u)); }

    topo_info *v = topo_unite(t, u);
    /* track sizing */
    v->size = t->size + u->size;

    /* reinsert both zones at v */
    if (z1) { insert_zone(v, z1); }
    if (z2) { insert_zone(v, z2); }
}

static bool
exists_alt_path(int x, int y, int z, block *a, block *b, ship_space *ship, int face)
{
    block *c;

    if (face != surface_xp) {
        c = ship->get_block(x+1, y, z);
        if (air_permeable(a->surfs[surface_xp]) && air_permeable(b->surfs[surface_xp]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
        c = ship->get_block(x-1, y, z);
        if (air_permeable(a->surfs[surface_xm]) && air_permeable(b->surfs[surface_xm]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
    }

    if (face != surface_yp) {
        c = ship->get_block(x, y+1, z);
        if (air_permeable(a->surfs[surface_yp]) && air_permeable(b->surfs[surface_yp]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
        c = ship->get_block(x, y-1, z);
        if (air_permeable(a->surfs[surface_ym]) && air_permeable(b->surfs[surface_ym]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
    }

    if (face != surface_zp) {
        c = ship->get_block(x, y, z+1);
        if (air_permeable(a->surfs[surface_zp]) && air_permeable(b->surfs[surface_zp]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
        c = ship->get_block(x, y, z-1);
        if (air_permeable(a->surfs[surface_zm]) && air_permeable(b->surfs[surface_zm]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
    }

    return false;
}

void
ship_space::update_topology_for_add_surface(int x, int y, int z, int px, int py, int pz, int face)
{
    /* can this surface even split (does it block atmo?) */
    if (air_permeable(get_block(x, y, z)->surfs[face]))
        return;

    /* collapse an obvious symmetry */
    if (face & 1) {
        /* symmetry */
        std::swap(x, px);
        std::swap(y, py);
        std::swap(z, pz);
        face ^= 1;
    }

    /* try to quickly prove that we don't divide space */
    if (exists_alt_path(x, y, z, get_block(x, y, z), get_block(px, py, pz), this, face)) {
        num_fast_nosplits++;
        return;
    }

    /* grab our air amount data before rebuild_topology invalidates the existing zones */
    zone_info *zone = get_zone_info(topo_find(get_topo_info(x, y, z)));
    float air_amount = zone ? zone->air_amount : 0.0f;

    /* we do need to split */
    rebuild_topology();

    topo_info *t1 = topo_find(get_topo_info(x, y, z));
    topo_info *t2 = topo_find(get_topo_info(px, py, pz));
    if (t1 == t2) {
        /* we blew it. we didn't actually split the space, but we did
         * all the work anyway. this is mostly interesting if you're
         * tweaking exists_alt_path. */
        num_false_splits++;
    }
    else if (zone) {
        /* at least one side was real before this split */

        /* fixup the zones for the split. we want to maintain the same pressure
         * we had on both sides, so distribute the mass */
        zone_info *z1 = get_zone_info(t1);
        if (!z1) {
            z1 = zones[t1] = new zone_info(0);
        }

        zone_info *z2 = get_zone_info(t2);
        if (!z2) {
            z2 = zones[t2] = new zone_info(0);
        }

        z1->air_amount = air_amount * t1->size / (t1->size + t2->size);
        z2->air_amount = air_amount - z1->air_amount;
    }
}

static glm::ivec3 dirs[] = {
    glm::ivec3(1, 0, 0),
    glm::ivec3(-1, 0, 0),
    glm::ivec3(0, 1, 0),
    glm::ivec3(0, -1, 0),
    glm::ivec3(0, 0, 1),
    glm::ivec3(0, 0, -1),
};


/* rebuild the ship topology. this is generally not the optimal thing -
 * we can dynamically rebuild parts of the topology cheaper based on
 * knowing the change that was made.
 */
void
ship_space::rebuild_topology()
{
    num_full_rebuilds++;

    /* 1/ initially, every block is its own subtree */
    for (auto it = chunks.begin(); it != chunks.end(); it++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    topo_info *t = it->second->topo.get(x, y, z);
                    t->p = t;
                    t->rank = 0;
                    t->size = 0;
                }
            }
        }
    }

    this->outside_topo_info.p = &this->outside_topo_info;
    this->outside_topo_info.rank = 0;
    this->outside_topo_info.size = 0;

    /* 2/ combine across air-permeable interfaces */
    for (auto it = chunks.begin(); it != chunks.end(); it++) {
        for (int z = 1; z < CHUNK_SIZE - 1; z++) {
            for (int y = 1; y < CHUNK_SIZE - 1; y++) {
                for (int x = 1; x < CHUNK_SIZE - 1; x++) {
                    block *bl = it->second->blocks.get(x, y, z);

                    for (int i = 0; i < 6; i++) {
                        if (air_permeable(bl->surfs[i])) {
                            glm::ivec3 offset = dirs[i];
                            topo_unite(it->second->topo.get(x, y, z),
                                       it->second->topo.get(x + offset.x, y + offset.y, z + offset.z));
                        }
                    }
                }
            }
        }

        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                block *bl = it->second->blocks.get(0, y, z);
                topo_info *to = it->second->topo.get(0, y, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + 0 + offset.x,
                                            CHUNK_SIZE * it->first.y + y + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(CHUNK_SIZE - 1, y, z);
                to = it->second->topo.get(CHUNK_SIZE - 1, y, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + CHUNK_SIZE - 1 + offset.x,
                                            CHUNK_SIZE * it->first.y + y + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, 0, z);
                to = it->second->topo.get(y, 0, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + 0 + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, CHUNK_SIZE - 1, z);
                to = it->second->topo.get(y, CHUNK_SIZE - 1, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + CHUNK_SIZE - 1 + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, z, 0);
                to = it->second->topo.get(y, z, 0);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + z + offset.y,
                                            CHUNK_SIZE * it->first.z + 0 + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, z, CHUNK_SIZE - 1);
                to = it->second->topo.get(y, z, CHUNK_SIZE - 1);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + z + offset.y,
                                            CHUNK_SIZE * it->first.z + CHUNK_SIZE - 1 + offset.z));
                    }
                }
            }
        }
    }

    /* 3/ finalize, and accumulate sizes */
    for (auto it = chunks.begin(); it != chunks.end(); it++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    topo_info *t = topo_find(it->second->topo.get(x, y, z));
                    t->size++;
                }
            }
        }
    }

    /* 4/ fixup zone_info */
    std::unordered_map<topo_info *, zone_info *> old_zones(std::move(zones));
    for (auto it : old_zones) {
        insert_zone(topo_find(it.first), it.second);
    }
}


bool
ship_space::validate()
{
    bool pass = true;

    for (auto ch : chunks) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    block *bl = ch.second->blocks.get(x, y, z);
                    for (int face = 0; face < 6; face++) {
                        glm::ivec3 offset = dirs[face];
                        glm::ivec3 other_coord(CHUNK_SIZE * ch.first.x + x + offset.x,
                                               CHUNK_SIZE * ch.first.y + y + offset.y,
                                               CHUNK_SIZE * ch.first.z + z + offset.z);
                        block *other = get_block(other_coord.x, other_coord.y, other_coord.z);

                        if (bl->surfs[face]) {
                            /* 1/ every surface must be consistent with its far side. this implies that the
                             *    far side *block* must also exist, so that the surface can
                             */
                            if (!other) {
                                printf("validate(): %d %d %d in nonexistent chunk, but far side of surface %d exists\n",
                                        other_coord.x, other_coord.y, other_coord.z, face ^ 1);
                                pass = false;
                            }
                            else if (other->surfs[face ^ 1] != bl->surfs[face]) {
                                printf("validate(): inconsistent surface %d %d %d face %d\n",
                                        other_coord.x, other_coord.y, other_coord.z, face ^ 1);
                                pass = false;
                            }

                            /* 2/ every surface must be supported by scaffolding on at least one side */
                            if (bl->type != block_support && (!other || other->type != block_support)) {
                                printf("validate(): %d %d %d face %d has no supporting scaffold\n",
                                        other_coord.x - offset.x, other_coord.y - offset.y,
                                        other_coord.z - offset.z, face);
                                pass = false;
                            }
                        }
                    }
                }
            }
        }
    }

    /* TODO: validate anything else we might have screwed up */
    if (pass) {
        printf("validate(): OK\n");
    }

    return pass;
}
