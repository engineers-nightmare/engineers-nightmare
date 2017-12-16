#include "ship_space.h"
#include <assert.h>
#include <math.h>


/* create an empty ship_space */
ship_space::ship_space(void)
    : mins(), maxs(),
      num_full_rebuilds(0), num_fast_unifys(0), num_fast_nosplits(0), num_false_splits(0)
{
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
ship_space::get_block(glm::ivec3 block)
{
    /* Within Block coordinates */
    int wb_x, wb_y, wb_z;
    glm::ivec3 ch;

    split_coord(block.x, &wb_x, &ch.x);
    split_coord(block.y, &wb_y, &ch.y);
    split_coord(block.z, &wb_z, &ch.z);

    chunk *c = this->get_chunk(ch);

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
ship_space::get_topo_info(glm::ivec3 block)
{
    /* Within Block coordinates */
    int wb_x, wb_y, wb_z;
    glm::ivec3 ch;

    split_coord(block.x, &wb_x, &ch.x);
    split_coord(block.y, &wb_y, &ch.y);
    split_coord(block.z, &wb_z, &ch.z);

    chunk *c = this->get_chunk(ch);

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
ship_space::get_chunk_containing(glm::ivec3 block)
{
    int chunk_x, chunk_y, chunk_z;

    split_coord(block.x, nullptr, &chunk_x);
    split_coord(block.y, nullptr, &chunk_y);
    split_coord(block.z, nullptr, &chunk_z);

    return this->get_chunk(glm::ivec3(chunk_x, chunk_y, chunk_z));
}

/* returns the chunk corresponding to the chunk coordinates (x, y, z)
 * note this is NOT using block coordinates
 */
chunk *
ship_space::get_chunk(glm::ivec3 ch)
{
    auto it = this->chunks.find(ch);
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


bool
ship_space::raycast_block(glm::vec3 o, glm::vec3 d, float max_reach_distance, raycast_stopping_rule stopping_rule, raycast_info_block *rc)
{
    /* implementation of the algorithm described in
     * http://www.cse.yorku.ca/~amana/research/grid.pdf
     */

    assert(rc);
    rc->hit = false;
    rc->t = 0.0f;

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

    bl = this->get_block(glm::ivec3(x,y,z));
    rc->inside = bl ? bl->type != block_empty && bl->type != block_untouched : false;

    int stepX = d.x > 0 ? 1 : -1;
    int stepY = d.y > 0 ? 1 : -1;
    int stepZ = d.z > 0 ? 1 : -1;

    float tDeltaX = fabsf(1/d.x);
    float tDeltaY = fabsf(1/d.y);
    float tDeltaZ = fabsf(1/d.z);

    float tMaxX = max_along_axis(o.x, d.x);
    float tMaxY = max_along_axis(o.y, d.y);
    float tMaxZ = max_along_axis(o.z, d.z);
    float t = 0;

    while (t < max_reach_distance) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                t = tMaxX;
                tMaxX += tDeltaX;
                nx = -stepX;
                ny = 0;
                nz = 0;
            }
            else {
                z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                nx = 0;
                ny = 0;
                nz = -stepZ;
            }
        }
        else {
            if (tMaxY < tMaxZ) {
                y += stepY;
                t = tMaxY;
                tMaxY += tDeltaY;
                nx = 0;
                ny = -stepY;
                nz = 0;
            }
            else {
                z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                nx = 0;
                ny = 0;
                nz = -stepZ;
            }
        }

        bl = this->get_block(glm::ivec3(x, y, z));
        if (!bl && !rc->inside){
            /* if there is no block then we are outside the grid
             * we still want to keep stepping until we either
             * hit a block within the grid or exceed our maximum
             * reach
             */
            continue;
        }

        if (stopping_rule & enter_exit_framing) {
            if (rc->inside ^ (bl && bl->type != block_empty && bl->type != block_untouched)) {
                rc->hit = true;
                rc->bl.x = x;
                rc->bl.y = y;
                rc->bl.z = z;
                rc->block = bl;
                rc->n.x = nx;
                rc->n.y = ny;
                rc->n.z = nz;
                rc->p.x = x + nx;
                rc->p.y = y + ny;
                rc->p.z = z + nz;
                rc->t = t;
		        rc->hitCoord = o + rc->t * d;
                return rc->hit;
            }
        }

        if (stopping_rule & cross_surface) {
            int index = normal_to_surface_index(nx, ny, nz);
            if (bl && bl->surfs[index]) {
                rc->hit = true;
                rc->bl.x = x;
                rc->bl.y = y;
                rc->bl.z = z;
                rc->block = bl;
                rc->n.x = nx;
                rc->n.y = ny;
                rc->n.z = nz;
                rc->p.x = x + nx;
                rc->p.y = y + ny;
                rc->p.z = z + nz;
                rc->t = t;
                rc->hitCoord = o + rc->t * d;
                return rc->hit;
            }
        }
    }
    return rc->hit;
}

/* ensure that the specified block_{x,y,z} can be fetched with a get_block
 *
 * this will instantiate a new containing chunk if necessary
 *
 * this will not instantiate or modify any other chunks
 */
block *
ship_space::ensure_block(glm::ivec3 block)
{
    glm::ivec3 ch;

    split_coord(block.x, nullptr, &ch.x);
    split_coord(block.y, nullptr, &ch.y);
    split_coord(block.z, nullptr, &ch.z);

    /* guarantee we have the size we need */
    this->ensure_chunk(ch);

    return get_block(block);
}

/* internal helper for creating chunks in a valid state.
 * all blocks within the newly-created chunk are connected to the outside
 * node in the atmo topology. this is the correct behavior for on-demand
 * chunk creation as you edit the world. clients doing bulk creation of
 * chunks should rebuild the atmo topology when they are finished making
 * changes.
 */
static chunk *
create_chunk(ship_space *ship)
{
    auto *ch = new chunk();

    /* All the topo nodes in the new chunk should be attached
     * to the outside node.
     */
    for (auto k = 0; k < CHUNK_SIZE; k++) {
        for (auto j = 0; j < CHUNK_SIZE; j++) {
            for (auto i = 0; i < CHUNK_SIZE; i++) {
                topo_info *t = ch->topo.get(i, j, k);
                t->p = &ship->outside_topo_info;
            }
        }
    }

    /* Adjust the size of the outside chunk. This is currently not
     * used for anything, but the consistency is nice and the cost is negligible.
     */
    ship->outside_topo_info.size += CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    return ch;
}

/* ensure that the specified chunk exists
 *
 * this will instantiate a new chunk if necessary -- any other possibly-enclosed
 * missing chunks to unconfuse the atmo system.
 */
chunk *
ship_space::ensure_chunk(glm::ivec3 v)
{
    /* automatically creates the entry if not present */
    auto &ch = this->chunks[v];
    if (!ch) {
        this->mins = glm::min(this->mins, v);
        this->maxs = glm::max(this->maxs, v);

        ch = create_chunk(this);

        /* ensure any other missing possibly-enclosed chunks exist too */
        for (auto k = this->mins.z + 1; k < this->maxs.z; k++) {
            for (auto j = this->mins.y + 1; j < this->maxs.y; j++) {
                for (auto i = this->mins.x + 1; i < this->maxs.x; i++) {
                    auto &other_ch = this->chunks[glm::ivec3(i, j, k)];
                    if (!other_ch) {
                        other_ch = create_chunk(this);
                    }
                }
            }
        }
    }

    return ch;
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
ship_space::update_topology_for_remove_surface(glm::ivec3 a, glm::ivec3 b)
{
    topo_info *t = topo_find(get_topo_info(a));
    topo_info *u = topo_find(get_topo_info(b));

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
        c = ship->get_block(glm::ivec3(x+1, y, z));
        if (air_permeable(a->surfs[surface_xp]) && air_permeable(b->surfs[surface_xp]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
        c = ship->get_block(glm::ivec3(x-1, y, z));
        if (air_permeable(a->surfs[surface_xm]) && air_permeable(b->surfs[surface_xm]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
    }

    if (face != surface_yp) {
        c = ship->get_block(glm::ivec3(x, y+1, z));
        if (air_permeable(a->surfs[surface_yp]) && air_permeable(b->surfs[surface_yp]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
        c = ship->get_block(glm::ivec3(x, y-1, z));
        if (air_permeable(a->surfs[surface_ym]) && air_permeable(b->surfs[surface_ym]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
    }

    if (face != surface_zp) {
        c = ship->get_block(glm::ivec3(x, y, z+1));
        if (air_permeable(a->surfs[surface_zp]) && air_permeable(b->surfs[surface_zp]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
        c = ship->get_block(glm::ivec3(x, y, z-1));
        if (air_permeable(a->surfs[surface_zm]) && air_permeable(b->surfs[surface_zm]) &&
                (!c || air_permeable(c->surfs[face])))
            return true;
    }

    return false;
}

/* todo: we should be able to calculate face */
void
ship_space::update_topology_for_add_surface(glm::ivec3 a, glm::ivec3 b, int face)
{
    /* can this surface even split (does it block atmo?) */
    if (air_permeable(get_block(a)->surfs[face]))
        return;

    /* collapse an obvious symmetry */
    if (face & 1) {
        /* symmetry */
        std::swap(a, b);
        face ^= 1;
    }

    /* try to quickly prove that we don't divide space */
    if (exists_alt_path(a.x, a.y, a.z, get_block(a), get_block(b), this, face)) {
        num_fast_nosplits++;
        return;
    }

    /* grab our air amount data before rebuild_topology invalidates the existing zones */
    zone_info *zone = get_zone_info(topo_find(get_topo_info(a)));
    float air_amount = zone ? zone->air_amount : 0.0f;

    /* we do need to split */
    rebuild_topology();

    topo_info *t1 = topo_find(get_topo_info(a));
    topo_info *t2 = topo_find(get_topo_info(b));
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
                            get_topo_info(CHUNK_SIZE * it->first + glm::ivec3(0, y, z) + offset));
                    }
                }

                bl = it->second->blocks.get(CHUNK_SIZE - 1, y, z);
                to = it->second->topo.get(CHUNK_SIZE - 1, y, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                            get_topo_info(CHUNK_SIZE * it->first + glm::ivec3(CHUNK_SIZE - 1, y, z) + offset));
                    }
                }

                bl = it->second->blocks.get(y, 0, z);
                to = it->second->topo.get(y, 0, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                            get_topo_info(CHUNK_SIZE * it->first + glm::ivec3(y, 0, z) + offset));
                    }
                }

                bl = it->second->blocks.get(y, CHUNK_SIZE - 1, z);
                to = it->second->topo.get(y, CHUNK_SIZE - 1, z);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                            get_topo_info(CHUNK_SIZE * it->first + glm::ivec3(y, CHUNK_SIZE - 1, z) + offset));
                    }
                }

                bl = it->second->blocks.get(y, z, 0);
                to = it->second->topo.get(y, z, 0);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                            get_topo_info(CHUNK_SIZE * it->first + glm::ivec3(y, z, 0) + offset));
                    }
                }

                bl = it->second->blocks.get(y, z, CHUNK_SIZE - 1);
                to = it->second->topo.get(y, z, CHUNK_SIZE - 1);

                for (int i = 0; i < 6; i++) {
                    if (air_permeable(bl->surfs[i])) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                            get_topo_info(CHUNK_SIZE * it->first + glm::ivec3(y, z, CHUNK_SIZE - 1) + offset));
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
                        glm::ivec3 other_coord = CHUNK_SIZE * ch.first + glm::ivec3(x, y, z) + offset;
                        block *other = get_block(other_coord);

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

                            /* 2/ every surface must be supported by framing on at least one side */
                            if (bl->type != block_frame && (!other || other->type != block_frame)) {
                                printf("validate(): %d %d %d face %d has no supporting frame\n",
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

/* todo: we should be able to calculate surface index */
void
ship_space::set_surface(glm::ivec3 a, glm::ivec3 b, surface_index index, surface_type st) {
    auto block = ensure_block(a);
    auto other_block = ensure_block(b);

    block->surfs[index] = st;
    get_chunk_containing(a)->render_chunk.valid = false;
    get_chunk_containing(a)->phys_chunk.valid = false;

    other_block->surfs[index ^ 1] = st;
    get_chunk_containing(b)->render_chunk.valid = false;
    get_chunk_containing(b)->phys_chunk.valid = false;

    if (st != surface_none) {
        update_topology_for_add_surface(a, b, index);
    }
    else {
        update_topology_for_remove_surface(a, b);
    }
}

bool ship_space::find_next_block(glm::ivec3 start, glm::ivec3 dir, unsigned limit, glm::ivec3 *found) {
    auto cur = start + dir;

    block *bl = nullptr;
    while ((bl = get_block(cur)) && bl->type != block_frame && limit > 0) {
        cur += dir;
        limit--;
    }

    if (bl && bl->type == block_frame) {
        *found = cur;
        return true;
    }

    return false;
}

block *ship_space::get_block_neighbor(glm::ivec3 block, enum surface_index si) {
    glm::ivec3 t = block + surface_index_to_normal(si);

    ensure_block(t);
    return get_block(t);
}

void ship_space::cut_out_cuboid(glm::ivec3 mins, glm::ivec3 maxs, surface_type type) {
    for (auto i = mins.x - 1; i <= maxs.x + 1; i++) {
        for (auto j = mins.y - 1; j <= maxs.y + 1; j++) {
            for (auto k = mins.z - 1; k <= maxs.z + 1; k++) {
                glm::ivec3 p(i, j, k);
                ensure_block(p);

                // Convert untouched surroundings to frame
                auto bl = get_block(p);
                if (bl->type == block_untouched)
                    bl->type = block_frame;

                get_chunk_containing(p)->phys_chunk.valid = false;
                get_chunk_containing(p)->render_chunk.valid = false;
            }
        }
    }

    for (auto i = mins.x; i <= maxs.x; i++) {
        for (auto j = mins.y; j <= maxs.y; j++) {
            for (auto k = mins.z; k <= maxs.z; k++) {
                glm::ivec3 p(i, j, k);
                remove_block(p);

                if (i == mins.x) {
                    auto b = p + surface_index_to_normal(surface_xm);
                    if (get_block(b)->type == block_frame) {
                        set_surface(b, p, surface_xp, type);
                    }
                }
                if (i == maxs.x) {
                    auto b = p + surface_index_to_normal(surface_xp);
                    if (get_block(b)->type == block_frame) {
                        set_surface(b, p, surface_xm, type);
                    }
                }

                if (j == mins.y) {
                    auto b = p + surface_index_to_normal(surface_ym);
                    if (get_block(b)->type == block_frame) {
                        set_surface(b, p, surface_yp, type);
                    }
                }
                if (j == maxs.y) {
                    auto b = p + surface_index_to_normal(surface_yp);
                    if (get_block(b)->type == block_frame) {
                        set_surface(b, p, surface_ym, type);
                    }
                }

                if (k == mins.z) {
                    auto b = p + surface_index_to_normal(surface_zm);
                    if (get_block(b)->type == block_frame) {
                        set_surface(b, p, surface_zp, type);
                    }
                }
                if (k == maxs.z) {
                    auto b = p + surface_index_to_normal(surface_zp);
                    if (get_block(b)->type == block_frame) {
                        set_surface(b, p, surface_zm, type);
                    }
                }
            }
        }
    }
}
