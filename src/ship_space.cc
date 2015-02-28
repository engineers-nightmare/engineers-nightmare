#include "ship_space.h"

block *
chunk::get_block(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= CHUNK_SIZE ||
        y >= CHUNK_SIZE ||
        z >= CHUNK_SIZE )
        return 0;

    return this->blocks.get(x, y, z);
}

/* returns a block or null
 * finds the block at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 */
block *
ship_space::get_block(unsigned int block_x, unsigned int block_y, unsigned int block_z)
{
    /* Within Block coordinates */
    unsigned int wb_x = block_x % CHUNK_SIZE;
    unsigned int wb_y = block_y % CHUNK_SIZE;
    unsigned int wb_z = block_z % CHUNK_SIZE;

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
ship_space::get_chunk_containing(unsigned int block_x, unsigned int block_y, unsigned int block_z)
{
    unsigned int chunk_x = block_x / CHUNK_SIZE;
    unsigned int chunk_y = block_y / CHUNK_SIZE;
    unsigned int chunk_z = block_z / CHUNK_SIZE;

    return this->get_chunk(chunk_x, chunk_y, chunk_z);
}

/* returns the chunk corresponding to the chunk coordinates (x, y, z)
 * note this is NOT using block coordinates
 */
chunk *
ship_space::get_chunk(unsigned int chunk_x, unsigned int chunk_y, unsigned int chunk_z)
{
    return this->chunks.get(chunk_x, chunk_y, chunk_z);
}


