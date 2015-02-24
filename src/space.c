#include "space.h"

block *
chunk::get_block(int x, int y, int z)
{
    if( x >= CHUNK_SIZE ||
        y >= CHUNK_SIZE ||
        z >= CHUNK_SIZE )
        return 0;

    return &( this->blocks.contents[x][y][z] );
}

