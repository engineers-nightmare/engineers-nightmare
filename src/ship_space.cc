#include "ship_space.h"
#include <new> /* placement new */
#include <assert.h>
#include <math.h>

ship_space::ship_space(unsigned int xd, unsigned int yd, unsigned int zd)
    : chunks(xd, yd, zd)
{
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
    int wb_x = abs(block_x % CHUNK_SIZE);
    int wb_y = abs(block_y % CHUNK_SIZE);
    int wb_z = abs(block_z % CHUNK_SIZE);

    chunk *c;

    c = this->get_chunk_containing(block_x, block_y, block_z);
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
    int chunk_x = block_x / CHUNK_SIZE;
    int chunk_y = block_y / CHUNK_SIZE;
    int chunk_z = block_z / CHUNK_SIZE;

    /* if we had negative chunk_{x,y,z} then we need
     * to adjust for the 0 bias of /
     *
     * say (0,0,-1) / 8 will become (0, 0, 0)
     * when we really want it to stay (0, 0, -1)
     *
     */
    chunk_x -= block_x < 0 ? 1 : 0;
    chunk_y -= block_y < 0 ? 1 : 0;
    chunk_z -= block_z < 0 ? 1 : 0;

    return this->get_chunk(chunk_x, chunk_y, chunk_z);
}

/* returns the chunk corresponding to the chunk coordinates (x, y, z)
 * note this is NOT using block coordinates
 */
chunk *
ship_space::get_chunk(int chunk_x, int chunk_y, int chunk_z)
{
    chunk **c = this->chunks.get(chunk_x, chunk_y, chunk_z);
    return c ? *c : NULL;
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
    *ss->chunks.get(0, 0, 0) = new chunk();
    *ss->chunks.get(1, 0, 0) = new chunk();
    *ss->chunks.get(0, 1, 0) = new chunk();
    *ss->chunks.get(1, 1, 0) = new chunk();

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

        if (bl->type != block_empty ^ rc->inside) {
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
 * this will trigger a resize and will instantiate a new containing chunk
 * if necessary
 *
 * this will not instantiate or modify any other chunks
 */
void
ship_space::ensure_block(int block_x, int block_y, int block_z)
{
    /* convert block to co-ords of containing chunk
     * the result of this may be negative and that is okay
     * as dynamic_grid handles the offsets for us
     */
    int chunk_x = block_x / CHUNK_SIZE,
        chunk_y = block_y / CHUNK_SIZE,
        chunk_z = block_z / CHUNK_SIZE;

    /* if we had negative chunk_{x,y,z} then we need
     * to adjust for the 0 bias of /
     *
     * say (0,0,-1) / 8 will become (0, 0, 0)
     * when we really want it to stay (0, 0, -1)
     *
     */
    chunk_x -= block_x < 0 ? 1 : 0;
    chunk_y -= block_y < 0 ? 1 : 0;
    chunk_z -= block_z < 0 ? 1 : 0;

    /* guarantee we have the size we need */
    this->chunks.ensure(chunk_x, chunk_y, chunk_z);

    /* need to now make sure our chunk is instantiated */

    /* capture our potential chunk */
    chunk **c = this->chunks.get(chunk_x, chunk_y, chunk_z);

    /* check we didn't get back garbage */
    if( ! c ){
        /* get returned null, die */
        printf("ship_space::ensure chunks.get(%d, %d, %d) returned null\n",
                chunk_x, chunk_y, chunk_z);
        errx(1, "ship_space::ensure chunks.get failed");
    }

    /* if null then instantiate */
    if( ! *c ){
        /* instantiate our chunk */
        *c = new chunk();
    }

    /* we are now all good
     * our DG contains the needed chunks
     * and the desired chunk is instantiated
     */
}


