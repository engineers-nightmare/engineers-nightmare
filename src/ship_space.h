#pragma once

#include "block.h"
#include "grid.h"

#include <stdio.h>

#define CHUNK_SIZE 8

struct chunk {
    /* with a CHUNK_SIZE of 8
     * we have 8^3 blocks
     * this means a chunk represents
     * 8m^3
     */
    grid_3d<block> blocks;

    chunk(void) : blocks(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE) {}
    block * get_block(unsigned int x, unsigned int y, unsigned int z);
};

struct ship_space {
    grid_3d<chunk> chunks;

    /* a ship_space of xd * yd * zd
     */
    ship_space(unsigned int xd, unsigned int yd, unsigned int zd);

    /* returns a block or null
     * finds the block at the position (x,y,z) within
     * the whole ship_space
     * will move across chunks
     */
    block * get_block(unsigned int block_x, unsigned int block_y, unsigned int block_z);

    /* returns the chunk containing the block denotated by (x, y, z)
     * or null
     */
    chunk * get_chunk_containing(unsigned int block_x, unsigned int block_y, unsigned int block_z);

    /* returns the chunk corresponding to the chunk coordinates (x, y, z)
     * note this is NOT using block coordinates
     */
    chunk * get_chunk(unsigned int chunk_x, unsigned int chunk_y, unsigned int chunk_z);
};


