#pragma once

#include "block.h"
#include "chunk.h"
#include "dynamic_grid.h"

#include <stdio.h>

struct raycast_info {
    bool hit;
    int x, y, z;            /* the block we hit */
    int nx, ny, nz;         /* the face normal we hit */
    int px, py, pz;         /* the block along the normal */
    struct block *block;
};

struct ship_space {
    dynamic_grid<chunk *> chunks;

    /* a ship_space of xd * yd * zd
     */
    ship_space(unsigned int xd, unsigned int yd, unsigned int zd);

    /* returns a block or null
     * finds the block at the position (x,y,z) within
     * the whole ship_space
     * will move across chunks
     */
    block * get_block(int block_x, int block_y, int block_z);

    /* returns the chunk containing the block denotated by (x, y, z)
     * or null
     */
    chunk * get_chunk_containing(int block_x, int block_y, int block_z);

    /* returns the chunk corresponding to the chunk coordinates (x, y, z)
     * note this is NOT using block coordinates
     */
    chunk * get_chunk(int chunk_x, int chunk_y, int chunk_z);

    /* returns a pointer to a new ship space
     * this ship space will have 2 x 2 rooms and will be 1 room tall
     * each room will have a floor and 4 walls of scaffolding
     * each room will have some doors on all 4 walls
     * there will be a floor of surfaces
     * and will otherwise be empty
     *
     * returns 0 on error
     */
    static ship_space * mock_ship_space(void);

    void raycast(float ox, float oy, float oz, float dx, float dy, float dz, raycast_info *rc);

    /* ensure that the specified block_{x,y,z} can be fetched with a get_block
     *
     * this will trigger a resize and will instantiate a new containing chunk
     * if necessary
     *
     * this will not instantiate or modify any other chunks
     */
    void ensure_block(int block_x, int block_y, int block_z);

    /* deprecated interface: will be dead soon
     *
     * resize ship to new dimensions
     * will ensure every stored chunk is still at the co-ords
     * of it's pre-resize location
     */
    void _resize(unsigned int nxd, unsigned int nyd, unsigned int nzd);

};


