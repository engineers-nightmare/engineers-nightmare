#include <stdio.h>
#include <assert.h>
#include "../src/ship_space.h"

void
simple(void)
{
    block *b;
    ship_space space;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                space.ensure_chunk(i, j, k);
            }
        }
    }

    assert( space.get_block(2*CHUNK_SIZE, 2*CHUNK_SIZE, 2*CHUNK_SIZE) == 0 );

    assert( space.get_block(0,0,0) != space.get_block(0,0,1) );



    /* write some data */
    b = space.get_block(0, 0, 0);
    assert(b != 0);
    b->type = block_empty;

    b = space.get_block(0, 0, 1);
    assert(b != 0);
    b->type = block_support;

    b = space.get_block(0, 8, 0);
    assert(b != 0);
    b->type = block_support;


    /* read some data */
    b = space.get_block(0, 0, 0);
    assert(b != 0);
    assert(b->type == block_empty);

    b = space.get_block(0, 0, 1);
    assert(b != 0);
    assert(b->type == block_support);

    b = space.get_block(0, 8, 0);
    assert(b != 0);
    assert(b->type == block_support);
}

void
ensure(void)
{
    ship_space space;
    space.ensure_chunk(0, 0, 0);

    /* check for chunk within range */
    assert( space.get_chunk(0, 0, 0) );

    /* check for chunks outside range */
    assert( 0 == space.get_chunk(0, 0, 1) );
    assert( 0 == space.get_chunk(0, 1, 0) );
    assert( 0 == space.get_chunk(1, 0, 0) );

    /* check for blocks within range */
    assert( space.get_block(0,0,0) );
    assert( space.get_block(4,3,1) );
    assert( space.get_block(7,7,7) );

    /* check for blocks outside range */
    assert( 0 == space.get_block(3,16,16) );
    assert( 0 == space.get_block(7,7,8) );
    assert( 0 == space.get_block(8,8,8) );

    /* our space currently contains blocks 0..7 for all 3 dims
     * force a resize to allow for 0..31 along on z dim
     * this should not modify the existing chunk
     * and should only instantiate 1 new chunk
     */
    space.ensure_block(7, 7, 31);

    /* check that we did not instantiate any other chunks */
    assert( 0 == space.get_block(3,16,16) );
    assert( 0 == space.get_block(7,7,8) );
    assert( 0 == space.get_block(8,8,8) );
    assert( 0 == space.get_block(7,7,23) );
    assert( 0 == space.get_block(0,0,-1) );
    assert( 0 == space.get_block(0,0,-7) );

    /* check we did instantiate the right chunk */
    assert( space.get_block(7,7,24) );
    assert( space.get_block(7,7,31) );


    /* now ensure into the negative */
    space.ensure_block(0, 0, -1);

    /* do some checking */
    assert( space.get_block(0,0,-1) );
    assert( space.get_block(0,0,-7) );

    /* and check some error cases */
    assert( 0 == space.get_block(0,0,-9) );
    assert( 0 == space.get_block(0,-1,0) );

}

/* some more quick and dirty 'testing'
 * mostly checking we compile and nothing
 * blows up obviously
 */
int
main(void)
{
    simple();
    ensure();
}
