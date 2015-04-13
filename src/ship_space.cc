#include "ship_space.h"
#include <new> /* placement new */
#include <assert.h>
#include <math.h>

ship_space::ship_space(unsigned int xd, unsigned int yd, unsigned int zd)
    : chunks(xd, yd, zd)
{
    int i=0, j=0, k=0;
    chunk *c;

    /* iterate through in z major / x minor order
     * yay access patterns
     */
    for( k=0; k<zd; ++k ){
        for( j=0; j<yd; ++j ){
            for( i=0; i<xd; ++i ){
                /* call new to construct our blocks */
                c = chunks.get(i, j, k);
                if( ! c ){
                    errx(1, "ship_space::ship_space : failed to initialise chunks");
                }
                new (c) chunk();
            }
        }
    }

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
    int wb_x = block_x % CHUNK_SIZE;
    int wb_y = block_y % CHUNK_SIZE;
    int wb_z = block_z % CHUNK_SIZE;

    chunk *c;

    c = this->get_chunk_containing(block_x, block_y, block_z);
    if( ! c )
        return 0;

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

    return this->get_chunk(chunk_x, chunk_y, chunk_z);
}

/* returns the chunk corresponding to the chunk coordinates (x, y, z)
 * note this is NOT using block coordinates
 */
chunk *
ship_space::get_chunk(int chunk_x, int chunk_y, int chunk_z)
{
    return this->chunks.get(chunk_x, chunk_y, chunk_z);
}

/* returns a pointer to a new ship space
 * this ship_space will have a floor and 4 walls of scaffolding
 * and will otherwise be empty
 *
 * returns 0 on error
 */
ship_space *
ship_space::mock_ship_space(void)
{
    /* new ship space of 1 chunk ^ 3 */
    ship_space * ss = new ship_space(1, 1, 1);
    unsigned int x=0, y=0, z=0;
    block *b = 0;

    for( z=0; z < 8; ++z ){
        for( y=0; y < 8; ++y ){
            for( x=0; x < 8; ++x ){

                b = ss->get_block(x, y, z);

                if( z == 0 ){
                    /* the floor */
                    b->type = block_support;
                } else if( y == 0 ){
                    /* one wall */
                    b->type = block_support;
                } else if( x == 0 ){
                    /* two wall */
                    b->type = block_support;
                } else if( x == 7 ){
                    /* three wall */
                    b->type = block_support;
                } else if( y == 7 ){
                    /* four wall */
                    b->type = block_support;
                } else {
                    b->type = block_empty;
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

    if (!this->get_block(x, y, z))
        return; /* not inside the grid */

    int stepX = dx > 0 ? 1 : -1;
    int stepY = dy > 0 ? 1 : -1;
    int stepZ = dz > 0 ? 1 : -1;

    float tDeltaX = fabsf(1/dx);
    float tDeltaY = fabsf(1/dy);
    float tDeltaZ = fabsf(1/dz);

    float tMaxX = max_along_axis(ox, dx);
    float tMaxY = max_along_axis(oy, dy);
    float tMaxZ = max_along_axis(oz, dz);

    for (;;) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                tMaxX += tDeltaX;
            }
            else {
                z += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        else {
            if (tMaxY < tMaxZ) {
                y += stepY;
                tMaxY += tDeltaY;
            }
            else {
                z += stepZ;
                tMaxZ += tDeltaZ;
            }
        }

        block *bl = this->get_block(x, y, z);
        if (!bl)
            return;

        if (bl->type != block_empty) {
            rc->hit = true;
            rc->x = x;
            rc->y = y;
            rc->z = z;
            rc->block = bl;
            return;
        }
    }
}
