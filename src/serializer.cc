#include "serializer.h"

/* take a block and serialise it into the buffer
 *
 * index must point at location in buffer to begin writing
 * serialize_block will increment index to point to next free spot
 *
 * caller must ensure the buffer contains enough space for this block
 * which is SERIALIZE_BLOCK_SIZE uint8_t(s)
 *
 * returns 0 on success
 * returns 1 on error
 */
unsigned int serialize_block(block *b, uint8_t *buffer, unsigned int *index)
{
    if( ! b      ||
        ! buffer ||
        ! index  ){
        errx(1, "serialize_block: given null argument");
        return 1;
    }

    /* format is
     * type
     * 6 surfaces
     * 6 surface spaces
     */


    buffer[(*index)++] = b->type;

    buffer[(*index)++] = b->surfs[surface_xp];
    buffer[(*index)++] = b->surfs[surface_xm];
    buffer[(*index)++] = b->surfs[surface_yp];
    buffer[(*index)++] = b->surfs[surface_ym];
    buffer[(*index)++] = b->surfs[surface_zp];
    buffer[(*index)++] = b->surfs[surface_zm];

    buffer[(*index)++] = b->surf_space[surface_xp];
    buffer[(*index)++] = b->surf_space[surface_xm];
    buffer[(*index)++] = b->surf_space[surface_yp];
    buffer[(*index)++] = b->surf_space[surface_ym];
    buffer[(*index)++] = b->surf_space[surface_zp];
    buffer[(*index)++] = b->surf_space[surface_zm];

    return 0;
}

/* take a buffer with a serialise block in it and return a
 * block (newly allocated - FIXME )
 *
 * index must point to start of block
 *
 * deseriaize_block will increment index past the end of the block
 *
 * returns *block on success
 * returns 0 on error
 */
block * deserialize_block(uint8_t *buffer, unsigned int *index)
{
    block *b = 0;

    if( ! buffer ||
        ! index  ){
        errx(1, "deserialize_block: given null args");
        return 0;
    }

    b = (block*) calloc(1, sizeof(block));

    if( ! b ){
        errx(1, "deserialize_block: calloc failed");
        return 0;
    }

    b->type = (block_type) buffer[(*index)++];

    b->surfs[surface_xp] = (surface_type) buffer[(*index)++];
    b->surfs[surface_xm] = (surface_type) buffer[(*index)++];
    b->surfs[surface_yp] = (surface_type) buffer[(*index)++];
    b->surfs[surface_ym] = (surface_type) buffer[(*index)++];
    b->surfs[surface_zp] = (surface_type) buffer[(*index)++];
    b->surfs[surface_zm] = (surface_type) buffer[(*index)++];

    b->surf_space[surface_xp] = (unsigned short) buffer[(*index)++];
    b->surf_space[surface_xm] = (unsigned short) buffer[(*index)++];
    b->surf_space[surface_yp] = (unsigned short) buffer[(*index)++];
    b->surf_space[surface_ym] = (unsigned short) buffer[(*index)++];
    b->surf_space[surface_zp] = (unsigned short) buffer[(*index)++];
    b->surf_space[surface_zm] = (unsigned short) buffer[(*index)++];

    return b;
}


/* take a chunk and serialise it into the buffer
 *
 * index must point at location in buffer to begin writing
 * serialize_chunk will increment index to point to next free spot
 *
 * caller must ensure the buffer contains enough space for this chunk
 * which is SERIALIZE_CHUNK_SIZE uint8_t(s)
 *
 * returns 0 on success
 * returns 1 on error
 */
unsigned int serialize_chunk(chunk *b, uint8_t *buffer, unsigned int *index)
{
    errx(1, "serialize_chunk: unimplemented");
    return 1;
}

/* take a buffer with a serialised chunk in it and return a
 * chunk (newly allocated - FIXME )
 *
 * index must point to start of chunk
 *
 * deseriaize_block will increment index past the end of the chunk
 *
 * returns *block on success
 * returns 0 on error
 */
chunk * deserialize_chunk(uint8_t *buffer, unsigned int *index)
{
    errx(1, "deserialize_chunk: unimplemented");
    return 0;
}


