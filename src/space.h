#pragma once

#include "block.h"
#include "grid.h"

#define CHUNK_SIZE 8

struct chunk {
    /* with a CHUNK_SIZE of 8
     * we have 8^3 blocks
     * this means a chunk represents
     * 8m^3
     */
    grid_3d<block, CHUNK_SIZE> blocks;

    block * get_block(int x, int y, int z);
};

/* a sub space containing N^3 chunks
 */
template <int N>
struct sub_space {
    grid_3d<chunk, N> chunks;

    /* returns a block or null
     * finds the block at the position (x,y,z) within
     * the whole sub_space
     * will move across chunks
     */
    block * get_block(int block_x, int block_y, int block_z);

};

template <int N>
block *
sub_space<N>::get_block(int block_x, int block_y, int block_z)
{
    int chunk_x, chunk_y, chunk_z;

    chunk_x = block_x / CHUNK_SIZE;
    block_x = block_x % CHUNK_SIZE;

    chunk_y = block_y / CHUNK_SIZE;
    block_y = block_y % CHUNK_SIZE;

    chunk_z = block_z / CHUNK_SIZE;
    block_z = block_z % CHUNK_SIZE;

    /* check bounds */
    if( chunk_x >= N          ||
        block_x >= CHUNK_SIZE ||
        chunk_y >= N          ||
        block_y >= CHUNK_SIZE ||
        chunk_z >= N          ||
        block_z >= CHUNK_SIZE )
        return 0;

    return this->chunks.contents[chunk_x][chunk_y][chunk_z].get_block(block_x, block_y, block_z);
}

