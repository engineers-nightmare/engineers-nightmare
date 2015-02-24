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
    block * get_block(int x, int y, int z){
        return 0;
    }

};

