#include "chunk.h"


chunk::chunk()
{
    memset(this->blocks.contents, block_empty, sizeof(this->blocks));
}

block *
chunk::get_block(unsigned int x, unsigned int y, unsigned int z)
{
    if( x >= CHUNK_SIZE ||
        y >= CHUNK_SIZE ||
        z >= CHUNK_SIZE ){
        return 0;
    }

    return this->blocks.get(x, y, z);
}
