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

    /* returns the chunk containing the block denotated by (x, y, z)
     * or null
     */
    chunk * get_chunk_containing(int block_x, int block_y, int block_z);

    /* returns the chunk corresponding to the chunk coordinates (x, y, z)
     * note this is NOT using block coordinates
     */
    chunk * get_chunk(int chunk_x, int chunk_y, int chunk_z);
};

/* returns a block or null
 * finds the block at the position (x,y,z) within
 * the whole sub_space
 * will move across chunks
 */
template <int N>
block *
sub_space<N>::get_block(int block_x, int block_y, int block_z)
{
    /* Within Block coordinates */
    int wb_x, wb_y, wb_z;

    chunk *c;

    wb_x = block_x % CHUNK_SIZE;

    wb_y = block_y % CHUNK_SIZE;

    wb_z = block_z % CHUNK_SIZE;

    c = this->get_chunk_containing(block_x, block_y, block_z);
    if( ! c )
        return 0;

    return c->get_block(wb_x, wb_y, wb_z);
}

/* returns the chunk containing the block denotated by (x, y, z)
 * or null
 */
template <int N>
chunk *
sub_space<N>::get_chunk_containing(int block_x, int block_y, int block_z)
{
    int chunk_x, chunk_y, chunk_z;

    chunk_x = block_x / CHUNK_SIZE;

    chunk_y = block_y / CHUNK_SIZE;

    chunk_z = block_z / CHUNK_SIZE;

    return this->get_chunk(chunk_x, chunk_y, chunk_z);
}

/* returns the chunk corresponding to the chunk coordinates (x, y, z)
 * note this is NOT using block coordinates
 */
template <int N>
chunk *
sub_space<N>::get_chunk(int chunk_x, int chunk_y, int chunk_z)
{
    if( chunk_x >= N ||
        chunk_y >= N ||
        chunk_z >= N )
        return 0;

    return &( this->chunks.contents[chunk_x][chunk_y][chunk_z] );
}


