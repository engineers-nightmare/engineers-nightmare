#pragma once

#include "block.h"
#include "fixed_grid.h"

#include <stdio.h>

#define CHUNK_SIZE 8

struct chunk {
    /* with a CHUNK_SIZE of 8
     * we have 8^3 blocks
     * this means a chunk represents
     * 8m^3
     */
    fixed_grid<block> blocks;

    chunk(void) : blocks(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE) {}
    block * get_block(unsigned int x, unsigned int y, unsigned int z);
};

