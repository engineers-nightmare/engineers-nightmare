#include "ship_space.h"
#include <new> /* placement new */
#include <assert.h>
#include <math.h>

/* create a ship space of x * y * z instantiated chunks */
ship_space::ship_space(unsigned int xd, unsigned int yd, unsigned int zd)
    : min_x(0), min_y(0), min_z(0)
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
    : min_x(0), min_y(0), min_z(0), max_x(0), max_y(0), max_z(0)
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

    return c->get_block(wb_x, wb_y, wb_z);
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

    if( this->chunks.count(v) ){
        return this->chunks[v];
    }

    return NULL;
}

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

                    b1->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    b2->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    b3->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    b4->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;

                } else if( y == 0 ){
                    /* we want a 2 height door at x == 3
                     * and a one height door a x == 5
                     */
                    if( (x == 3     && z == 1) ||
                        (x == 3     && z == 2) ||
                        (x == 5     && z == 1) ){
                        /* a door */
                        b1->type = block_empty;
                        b2->type = block_empty;
                        b3->type = block_empty;
                        b4->type = block_empty;
                    } else {
                        /* a wall */
                        b1->type = block_support;
                        b2->type = block_support;
                        b3->type = block_support;
                        b4->type = block_support;

                        if( x != 0 &&
                            x != 7 ){
                            /* add surfaces to inside of walls */
                            b1->surfs[surface_yp] = surface_wall;
                            b2->surfs[surface_yp] = surface_wall;
                            b3->surfs[surface_yp] = surface_wall;
                            b4->surfs[surface_yp] = surface_wall;
                        }

                        /* add surfaces to inside of doorways */
                        if( (x == 3 && z == 3) ||
                            (x == 5 && z == 2) ){
                            /* tops */
                            b1->surfs[surface_zm] = surface_wall;
                            b2->surfs[surface_zm] = surface_wall;
                            b3->surfs[surface_zm] = surface_wall;
                            b4->surfs[surface_zm] = surface_wall;
                        }
                        if( (x == 2 && z == 1) ||
                            (x == 2 && z == 2) ||
                            (x == 4 && z == 1) ){
                            /* minus */
                            b1->surfs[surface_xp] = surface_wall;
                            b2->surfs[surface_xp] = surface_wall;
                            b3->surfs[surface_xp] = surface_wall;
                            b4->surfs[surface_xp] = surface_wall;
                        }
                        if( (x == 4 && z == 2) ||
                            (x == 4 && z == 1) ||
                            (x == 6 && z == 1) ){
                            /* plus */
                            b1->surfs[surface_xm] = surface_wall;
                            b2->surfs[surface_xm] = surface_wall;
                            b3->surfs[surface_xm] = surface_wall;
                            b4->surfs[surface_xm] = surface_wall;
                        }

                    }
                } else if( x == 0 ){
                    /* we want a 2 height door at y == 3
                     * and a one height door a y == 5
                     */
                    if( (y == 3     && z == 1) ||
                        (y == 3     && z == 2) ||
                        (y == 5     && z == 1) ){
                        /* a wall */
                        b1->type = block_empty;
                        b2->type = block_empty;
                        b3->type = block_empty;
                        b4->type = block_empty;
                    } else {
                        /* a wall */
                        b1->type = block_support;
                        b2->type = block_support;
                        b3->type = block_support;
                        b4->type = block_support;

                        if( y != 0 &&
                            y != 7 ){
                            /* add surfaces to inside of walls */
                            b1->surfs[surface_xp] = surface_wall;
                            b2->surfs[surface_xp] = surface_wall;
                            b3->surfs[surface_xp] = surface_wall;
                            b4->surfs[surface_xp] = surface_wall;
                        }

                        /* add surfaces to inside of doorways */
                        if( (y == 3 && z == 3) ||
                            (y == 5 && z == 2) ){
                            /* tops */
                            b1->surfs[surface_zm] = surface_wall;
                            b2->surfs[surface_zm] = surface_wall;
                            b3->surfs[surface_zm] = surface_wall;
                            b4->surfs[surface_zm] = surface_wall;
                        }
                        if( (y == 2 && z == 1) ||
                            (y == 2 && z == 2) ||
                            (y == 4 && z == 1) ){
                            /* minus */
                            b1->surfs[surface_yp] = surface_wall;
                            b2->surfs[surface_yp] = surface_wall;
                            b3->surfs[surface_yp] = surface_wall;
                            b4->surfs[surface_yp] = surface_wall;
                        }
                        if( (y == 4 && z == 2) ||
                            (y == 4 && z == 1) ||
                            (y == 6 && z == 1) ){
                            /* plus */
                            b1->surfs[surface_ym] = surface_wall;
                            b2->surfs[surface_ym] = surface_wall;
                            b3->surfs[surface_ym] = surface_wall;
                            b4->surfs[surface_ym] = surface_wall;
                        }

                    }
                } else if( x == 7 ){
                    /* we want a 2 height door at x == 3
                     * and a one height door a x == 5
                     */
                    if( (y == 3     && z == 1) ||
                        (y == 3     && z == 2) ||
                        (y == 5     && z == 1) ){
                        /* a door */
                        b1->type = block_empty;
                        b2->type = block_empty;
                        b3->type = block_empty;
                        b4->type = block_empty;
                    } else {
                        /* a wall */
                        b1->type = block_support;
                        b2->type = block_support;
                        b3->type = block_support;
                        b4->type = block_support;

                        if( y != 0 &&
                            y != 7 ){
                            /* add surfaces to inside of walls */
                            b1->surfs[surface_xm] = surface_wall;
                            b2->surfs[surface_xm] = surface_wall;
                            b3->surfs[surface_xm] = surface_wall;
                            b4->surfs[surface_xm] = surface_wall;
                        }

                        /* add surfaces to inside of doorways */
                        if( (y == 3 && z == 3) ||
                            (y == 5 && z == 2) ){
                            /* tops */
                            b1->surfs[surface_zm] = surface_wall;
                            b2->surfs[surface_zm] = surface_wall;
                            b3->surfs[surface_zm] = surface_wall;
                            b4->surfs[surface_zm] = surface_wall;
                        }
                        if( (y == 2 && z == 1) ||
                            (y == 2 && z == 2) ||
                            (y == 4 && z == 1) ){
                            /* minus */
                            b1->surfs[surface_yp] = surface_wall;
                            b2->surfs[surface_yp] = surface_wall;
                            b3->surfs[surface_yp] = surface_wall;
                            b4->surfs[surface_yp] = surface_wall;
                        }
                        if( (y == 4 && z == 2) ||
                            (y == 4 && z == 1) ||
                            (y == 6 && z == 1) ){
                            /* plus */
                            b1->surfs[surface_ym] = surface_wall;
                            b2->surfs[surface_ym] = surface_wall;
                            b3->surfs[surface_ym] = surface_wall;
                            b4->surfs[surface_ym] = surface_wall;
                        }

                    }
                } else if( y == 7 ){
                    /* we want a 2 height door at y == 3
                     * and a one height door a y == 5
                     */
                    if( (x == 3     && z == 1) ||
                        (x == 3     && z == 2) ||
                        (x == 5     && z == 1) ){
                        /* a door */
                        b1->type = block_empty;
                        b2->type = block_empty;
                        b3->type = block_empty;
                        b4->type = block_empty;
                    } else {
                        /* a wall */
                        b1->type = block_support;
                        b2->type = block_support;
                        b3->type = block_support;
                        b4->type = block_support;

                        if( x != 0 &&
                            x != 7 ){
                            /* add surfaces to inside of walls */
                            b1->surfs[surface_ym] = surface_wall;
                            b2->surfs[surface_ym] = surface_wall;
                            b3->surfs[surface_ym] = surface_wall;
                            b4->surfs[surface_ym] = surface_wall;
                        }

                        /* add surfaces to inside of doorways */
                        if( (x == 3 && z == 3) ||
                            (x == 5 && z == 2) ){
                            /* tops */
                            b1->surfs[surface_zm] = surface_wall;
                            b2->surfs[surface_zm] = surface_wall;
                            b3->surfs[surface_zm] = surface_wall;
                            b4->surfs[surface_zm] = surface_wall;
                        }
                        if( (x == 2 && z == 1) ||
                            (x == 2 && z == 2) ||
                            (x == 4 && z == 1) ){
                            /* minus */
                            b1->surfs[surface_xp] = surface_wall;
                            b2->surfs[surface_xp] = surface_wall;
                            b3->surfs[surface_xp] = surface_wall;
                            b4->surfs[surface_xp] = surface_wall;
                        }
                        if( (x == 4 && z == 2) ||
                            (x == 4 && z == 1) ||
                            (x == 6 && z == 1) ){
                            /* plus */
                            b1->surfs[surface_xm] = surface_wall;
                            b2->surfs[surface_xm] = surface_wall;
                            b3->surfs[surface_xm] = surface_wall;
                            b4->surfs[surface_xm] = surface_wall;
                        }

                    }
                } else {
                    b1->type = block_empty;
                    b2->type = block_empty;
                    b3->type = block_empty;
                    b4->type = block_empty;
                }

            }
        }
    }

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


void
ship_space::raycast(float ox, float oy, float oz, float dx, float dy, float dz, raycast_info *rc)
{
    /* implementation of the algorithm described in
     * http://www.cse.yorku.ca/~amana/research/grid.pdf
     */

    assert(rc);
    rc->hit = false;

    int x = (int)ox;
    int y = (int)oy;
    int z = (int)oz;

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

    /* this counting here ensures our reach distance
     * FIXME this will need to be tweaked
     */
    for (int i = 0; i < 6; ++i) {
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
        if (!bl){
            /* if there is no block then we are outside the grid
             * we still want to keep stepping until we either
             * hit a block within the grid or exceed our maximum
             * reach
             */
            continue;
        }

        if (bl->type != (block_empty ^ rc->inside)) {
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


