#pragma once

#include <stdint.h>

#include "block.h"
#include "chunk.h"
#include "ship_space.h"

#define SERIALIZE_BLOCK_SIZE 13

/* chunk serializaton currently only handles blocks
 * FIXME entititles
 */
#define SERIALIZE_CHUNK_SIZE (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * SERIALIZE_BLOCK_SIZE)

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
unsigned int serialize_block(block *b, uint8_t *buffer, unsigned int *index);

/* take a buffer containing a serialized block in it and return a
 * newly allocated block
 *
 * index must point to start of block
 *
 * deserialize_block will increment index past the end of the block
 *
 * returns *block on success
 * returns 0 on error
 */
block * deserialize_block(uint8_t *buffer, unsigned int *index);

/* take a buffer containing a serialized block and a pointer to a block
 * set block based on contents in serialized buffer
 *
 * index must point to start of block
 *
 * deseriaize_to_block will increment index past the end of the block
 *
 * returns 0 on success
 * returns 1 on error
 */
unsigned int deserialize_to_block(block *b, uint8_t *buffer, unsigned int *index);

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
unsigned int serialize_chunk(chunk *b, uint8_t *buffer, unsigned int *index);

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
chunk * deserialize_chunk(uint8_t *buffer, unsigned int *index);


