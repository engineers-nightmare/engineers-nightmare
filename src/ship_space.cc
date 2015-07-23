#include "ship_space.h"
#include <assert.h>
#include <math.h>
#include <algorithm>

/* create a ship space of x * y * z instantiated chunks */
ship_space::ship_space(unsigned int xd, unsigned int yd, unsigned int zd)
    : min_x(0), min_y(0), min_z(0), topo_dirty(true)
{
    unsigned int x = 0,
                 y = 0,
                 z = 0;

    for( x = 0; x < xd; ++x ){
        for( y = 0; y < yd; ++y ){
            for( z = 0; z < zd; ++z ){
                glm::ivec3 v(x, y, z);
                this->chunks[v] = new chunk();
            }
        }
    }

    /* dim is exclusive, max is inclusive
     * so subtract 1 and store
     */
    this->max_x = xd - 1;
    this->max_y = yd - 1;
    this->max_z = zd - 1;
}

/* create an empty ship_space */
ship_space::ship_space(void)
    : min_x(0), min_y(0), min_z(0), max_x(0), max_y(0), max_z(0), topo_dirty(true)
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

/* returns the chunk containing the block denotated by (x, y, z)
 * or null
 */
chunk *
ship_space::get_chunk_containing(int block_x, int block_y, int block_z)
{
    int chunk_x, chunk_y, chunk_z;

    split_coord(block_x, NULL, &chunk_x);
    split_coord(block_y, NULL, &chunk_y);
    split_coord(block_z, NULL, &chunk_z);

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

    return NULL;
}

/* given the x, y, z of a block and the surface we are interested in,
 * find the co-ords the correspond to the block along the normal of that surace
 *
 * tx, ty, and tz are out params
 */
static void
find_neighbor(int fx, int fy, int fz, enum surface_index si, int *tx, int *ty, int *tz){
    if( ! tx ||
        ! ty ||
        ! tz ){
        errx(1, "ship_space.c: find_neighbor tx, ty or tz null");
        return;
    }

    *tx = fx;
    *ty = fy;
    *tz = fz;

    switch( si ){
        case surface_xp:
            ++*tx;
            break;

        case surface_xm:
            --*tx;
            break;

        case surface_yp:
            ++*ty;
            break;

        case surface_ym:
            --*ty;
            break;

        case surface_zp:
            ++*tz;
            break;

        case surface_zm:
            --*tz;
            break;

        case face_count:
            errx(1, "ship_space.c: find_neighbor supplied surface_index of type 'face_count'");
            break;

        default:
            errx(1, "ship_space.c: find_neighbor supplied surface_index of unknown type");
            break;
    }
}

/* returns a block
 * finds the block at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 * will call ensure_block if needed
 */
block *
ship_space::ensure_and_get_block(int block_x, int block_y, int block_z){
    block *b = 0;

    this->ensure_block(block_x, block_y, block_z);
    b = this->get_block(block_x, block_y, block_z);

    if( ! b ){
        errx(1, "ship_space::ensure_and_get_block: call to get_block failed");
    }

    return b;
}

/* returns the neighbor of a block along a given suface's normal
 * finds the block at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 * will call ensure_block if needed
 */
block *
ship_space::get_block_neighbor(int block_x, int block_y, int block_z, enum surface_index si){
    int tx, ty, tz;
    find_neighbor(block_x, block_y, block_z, si, &tx, &ty, &tz);
    return this->ensure_and_get_block(tx, ty, tz);
}

/* I am lazy
 * we plan to nuke mock ship space once we have map loading/saving
 *
 * Set Neighbour for block N
 */
#define sn1(si, type) (ss->get_block_neighbor(x,   y,   z, si)->surfs[si ^ 1] = type)
#define sn2(si, type) (ss->get_block_neighbor(x,   y+8, z, si)->surfs[si ^ 1] = type)
#define sn3(si, type) (ss->get_block_neighbor(x+8, y,   z, si)->surfs[si ^ 1] = type)
#define sn4(si, type) (ss->get_block_neighbor(x+8, y+8, z, si)->surfs[si ^ 1] = type)

/* returns a pointer to a new ship space
 * this ship space will have 2 x 2 rooms and will be 1 room tall
 * each room will have a floor and 4 walls of scaffolding
 * each room will have some doors on all 4 walls
 * there will be a floor of surfaces
 * and will otherwise be empty
 *
 * returns 0 on error
 */
ship_space *
ship_space::mock_ship_space(void)
{
    /* new ship space of 2 * 2 * 1*/
    ship_space * ss = new ship_space(2, 2, 1);

    unsigned int x=0, y=0, z=0;
    block *b1 = 0;
    block *b2 = 0;
    block *b3 = 0;
    block *b4 = 0;

    /* LET THIS SERVE AS MOTIVATION FOR NEEDING MAP LOAD AND SAVE */

    /* first pass build complete outer shell for each chunk */
    for( z=0; z < 8; ++z ){
        for( y=0; y < 8; ++y ){
            for( x=0; x < 8; ++x ){

                b1 = ss->get_block(x,   y,   z);
                b2 = ss->get_block(x,   y+8, z);
                b3 = ss->get_block(x+8, y,   z);
                b4 = ss->get_block(x+8, y+8, z);

                if( z == 0 ){
                    /* the floor */
                    b1->type = block_support;
                    b2->type = block_support;
                    b3->type = block_support;
                    b4->type = block_support;

                    /* add surfaces to inside */
                    b1->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    sn1(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));
                    b2->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    sn2(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));
                    b3->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    sn3(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));
                    b4->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    sn4(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));

                    /* add surfaces to outside */
                    b1->surfs[surface_zm] = surface_wall;
                    sn1(surface_zm, surface_wall);
                    b2->surfs[surface_zm] = surface_wall;
                    sn2(surface_zm, surface_wall);
                    b3->surfs[surface_zm] = surface_wall;
                    sn3(surface_zm, surface_wall);
                    b4->surfs[surface_zm] = surface_wall;
                    sn4(surface_zm, surface_wall);

                } else if( z == 7 ){
                    /* the roof */
                    b1->type = block_support;
                    b2->type = block_support;
                    b3->type = block_support;
                    b4->type = block_support;

                    /* add surfaces to outside */
                    b1->surfs[surface_zp] = surface_wall;
                    sn1(surface_zp, surface_wall);
                    b2->surfs[surface_zp] = surface_wall;
                    sn2(surface_zp, surface_wall);
                    b3->surfs[surface_zp] = surface_wall;
                    sn3(surface_zp, surface_wall);
                    b4->surfs[surface_zp] = surface_wall;
                    sn4(surface_zp, surface_wall);

                    /* add surfaces to inside */
                    b1->surfs[surface_zm] = surface_wall;
                    sn1(surface_zm, surface_wall);
                    b2->surfs[surface_zm] = surface_wall;
                    sn2(surface_zm, surface_wall);
                    b3->surfs[surface_zm] = surface_wall;
                    sn3(surface_zm, surface_wall);
                    b4->surfs[surface_zm] = surface_wall;
                    sn4(surface_zm, surface_wall);

                } else if( y == 0 || y == 7 ){
                    /* a wall */
                    b1->type = block_support;
                    b2->type = block_support;
                    b3->type = block_support;
                    b4->type = block_support;

                    /* add surfaces to one side */
                    b1->surfs[surface_yp] = surface_wall;
                    sn1(surface_yp, surface_wall);
                    b2->surfs[surface_yp] = surface_wall;
                    sn2(surface_yp, surface_wall);
                    b3->surfs[surface_yp] = surface_wall;
                    sn3(surface_yp, surface_wall);
                    b4->surfs[surface_yp] = surface_wall;
                    sn4(surface_yp, surface_wall);

                    /* add surfaces to other side */
                    b1->surfs[surface_ym] = surface_wall;
                    sn1(surface_ym, surface_wall);
                    b2->surfs[surface_ym] = surface_wall;
                    sn2(surface_ym, surface_wall);
                    b3->surfs[surface_ym] = surface_wall;
                    sn3(surface_ym, surface_wall);
                    b4->surfs[surface_ym] = surface_wall;
                    sn4(surface_ym, surface_wall);

                } else if( x == 0 || x == 7 ){
                    /* a wall */
                    b1->type = block_support;
                    b2->type = block_support;
                    b3->type = block_support;
                    b4->type = block_support;

                    /* add surfaces to one side */
                    b1->surfs[surface_xp] = surface_wall;
                    sn1(surface_xp, surface_wall);
                    b2->surfs[surface_xp] = surface_wall;
                    sn2(surface_xp, surface_wall);
                    b3->surfs[surface_xp] = surface_wall;
                    sn3(surface_xp, surface_wall);
                    b4->surfs[surface_xp] = surface_wall;
                    sn4(surface_xp, surface_wall);

                    /* add surfaces to other side */
                    b1->surfs[surface_xm] = surface_wall;
                    sn1(surface_xm, surface_wall);
                    b2->surfs[surface_xm] = surface_wall;
                    sn2(surface_xm, surface_wall);
                    b3->surfs[surface_xm] = surface_wall;
                    sn3(surface_xm, surface_wall);
                    b4->surfs[surface_xm] = surface_wall;
                    sn4(surface_xm, surface_wall);

                } else {
                    b1->type = block_empty;
                    b2->type = block_empty;
                    b3->type = block_empty;
                    b4->type = block_empty;
                }

            }
        }
    }
    /* *******
     * chunks:
     * y
     * ^
     * |3 4
     * |1 2
     * \---> x
     *
     * we want a doorway between chunks:
     *  1 -> 2
     *  1 -> 3
     *  2 -> 4
     *  3 -> 4
     *
     *  FIXME we also want to add a light in each chunk
     *
     *  FIXME for each surface we remove below we must also remove the adjacent facing surface
     */

    {
        chunk * c1 = ss->get_chunk(0, 0, 0);

        b1 = c1->blocks.get(4, 7, 0);
        b1->surfs[surface_yp] = surface_none;
        b1->surfs[surface_ym] = surface_none;

        b2 = c1->blocks.get(7, 4, 0);
        b1->surfs[surface_xp] = surface_none;
        b1->surfs[surface_xm] = surface_none;
    }

    {
        chunk * c2 = ss->get_chunk(1, 0, 0);

        b1 = c2->blocks.get(4, 7, 0);
        b1->surfs[surface_yp] = surface_none;
        b1->surfs[surface_ym] = surface_none;

        b2 = c2->blocks.get(0, 4, 0);
        b1->surfs[surface_xp] = surface_none;
        b1->surfs[surface_xm] = surface_none;
    }

    {
        chunk * c3 = ss->get_chunk(0, 1, 0);

        b1 = c3->blocks.get(7, 4, 0);
        b1->surfs[surface_xp] = surface_none;
        b1->surfs[surface_xm] = surface_none;

        b2 = c3->blocks.get(4, 0, 0);
        b1->surfs[surface_yp] = surface_none;
        b1->surfs[surface_ym] = surface_none;
    }

    {
        chunk * c4 = ss->get_chunk(1, 1, 0);

        b1 = c4->blocks.get(0, 4, 0);
        b1->surfs[surface_yp] = surface_none;
        b1->surfs[surface_ym] = surface_none;

        b2 = c4->blocks.get(4, 0, 0);
        b1->surfs[surface_xp] = surface_none;
        b1->surfs[surface_xm] = surface_none;
    }

    /* FIXME skin 'inside' of doorways */

    return ss;
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
ship_space::raycast(float ox, float oy, float oz, float dx, float dy, float dz, raycast_info *rc)
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
    int x = (int)(ox < 0 ? ox - 1: ox);
    int y = (int)(oy < 0 ? oy - 1: oy);
    int z = (int)(oz < 0 ? oz - 1: oz);

    int nx = 0;
    int ny = 0;
    int nz = 0;

    block *bl = 0;

    bl = this->get_block(x,y,z);
    rc->inside = bl ? bl->type != block_empty : 0;

    int stepX = dx > 0 ? 1 : -1;
    int stepY = dy > 0 ? 1 : -1;
    int stepZ = dz > 0 ? 1 : -1;

    float tDeltaX = fabsf(1/dx);
    float tDeltaY = fabsf(1/dy);
    float tDeltaZ = fabsf(1/dz);

    float tMaxX = max_along_axis(ox, dx);
    float tMaxY = max_along_axis(oy, dy);
    float tMaxZ = max_along_axis(oz, dz);

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
void
ship_space::ensure_block(int block_x, int block_y, int block_z)
{
    int chunk_x, chunk_y, chunk_z;

    split_coord(block_x, NULL, &chunk_x);
    split_coord(block_y, NULL, &chunk_y);
    split_coord(block_z, NULL, &chunk_z);

    /* guarantee we have the size we need */
    this->ensure_chunk(chunk_x, chunk_y, chunk_z);
}

/* ensure that the specified chunk exists
 *
 * this will instantiate a new chunk if necessary
 *
 * this will not instantiate or modify any other chunks
 */
void
ship_space::ensure_chunk(int chunk_x, int chunk_y, int chunk_z)
{
    glm::ivec3 v(chunk_x, chunk_y, chunk_z);

    /* if count is 0 then we do not contain this key */
    if( ! this->chunks.count(v) ){
        /* if this is an insert then we need to also
         * keep track of our min/max bounds
         */
        this->_maintain_bounds(chunk_x, chunk_y, chunk_z);

        /* operator[] will create an element */
        this->chunks[v] = new chunk();
    }
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
static void
topo_unite(topo_info *from, topo_info *to)
{
    from = topo_find(from);
    to = topo_find(to);

    /* already in same subtee? */
    if (from == to) return;

    if (from->rank < to->rank) {
        from->p = to;
    } else if (from->rank > to->rank) {
        to->p = from;
    } else {
        /* merging two rank-r subtrees produces a rank-r+1 subtree. */
        to->p = from;
        from->rank++;
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
    if (!topo_dirty)
        return;
    topo_dirty = false;

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

                    /* TODO: proper air-permeability query -- soon it will be not just walls! */
                    for (int i = 0; i < 6; i++) {
                        if (bl->surfs[i] != surface_wall) {
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

                /* TODO: proper air-permeability query -- soon it will be not just walls! */
                for (int i = 0; i < 6; i++) {
                    if (bl->surfs[i] != surface_wall) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + 0 + offset.x,
                                            CHUNK_SIZE * it->first.y + y + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(CHUNK_SIZE - 1, y, z);
                to = it->second->topo.get(CHUNK_SIZE - 1, y, z);

                /* TODO: proper air-permeability query -- soon it will be not just walls! */
                for (int i = 0; i < 6; i++) {
                    if (bl->surfs[i] != surface_wall) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + CHUNK_SIZE - 1 + offset.x,
                                            CHUNK_SIZE * it->first.y + y + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, 0, z);
                to = it->second->topo.get(y, 0, z);

                /* TODO: proper air-permeability query -- soon it will be not just walls! */
                for (int i = 0; i < 6; i++) {
                    if (bl->surfs[i] != surface_wall) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + 0 + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, CHUNK_SIZE - 1, z);
                to = it->second->topo.get(y, CHUNK_SIZE - 1, z);

                /* TODO: proper air-permeability query -- soon it will be not just walls! */
                for (int i = 0; i < 6; i++) {
                    if (bl->surfs[i] != surface_wall) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + CHUNK_SIZE - 1 + offset.y,
                                            CHUNK_SIZE * it->first.z + z + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, z, 0);
                to = it->second->topo.get(y, z, 0);

                /* TODO: proper air-permeability query -- soon it will be not just walls! */
                for (int i = 0; i < 6; i++) {
                    if (bl->surfs[i] != surface_wall) {
                        glm::ivec3 offset = dirs[i];
                        topo_unite(to,
                              get_topo_info(CHUNK_SIZE * it->first.x + y + offset.x,
                                            CHUNK_SIZE * it->first.y + z + offset.y,
                                            CHUNK_SIZE * it->first.z + 0 + offset.z));
                    }
                }

                bl = it->second->blocks.get(y, z, CHUNK_SIZE - 1);
                to = it->second->topo.get(y, z, CHUNK_SIZE - 1);

                /* TODO: proper air-permeability query -- soon it will be not just walls! */
                for (int i = 0; i < 6; i++) {
                    if (bl->surfs[i] != surface_wall) {
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
}
