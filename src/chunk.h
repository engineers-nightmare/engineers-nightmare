#pragma once

#include "block.h"
#include "fixed_cube.h"

#define CHUNK_SIZE 8

struct chunk {
    /* with a CHUNK_SIZE of 8
     * we have 8^3 blocks
     * this means a chunk represents
     * 8m^3
     */
    fixed_cube<block, CHUNK_SIZE> blocks;

    block * get_block(unsigned int x, unsigned int y, unsigned int z);
};

