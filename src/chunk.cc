#include "chunk.h"


chunk::chunk()
{
    for (unsigned int z = 0; z < CHUNK_SIZE; ++z)
    {
        for (unsigned int y = 0; y < CHUNK_SIZE; ++y)
        {
            for (unsigned int x = 0; x < CHUNK_SIZE; ++x)
            {
                block *block = this->blocks.get(x, y, z);
                block->type = block_empty;
                for (int s = 0; s < face_count; ++s)
                {
                    block->surfs[s] = surface_none;
                }
            }
        }
    }
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
