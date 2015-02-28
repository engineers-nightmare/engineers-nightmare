#include <stdio.h>
#include <assert.h>
#include "../src/ship_space.h"

/* some more quick and dirty 'testing'
 * mostly checking we compile and nothing
 * blows up obviously
 */
int
main(void)
{
    block *b;
    ship_space space(2, 2, 2);

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
    b->yp = surface_none;


    /* read some data */
    b = space.get_block(0, 0, 0);
    assert(b != 0);
    assert(b->type == block_empty);

    b = space.get_block(0, 0, 1);
    assert(b != 0);
    assert(b->type == block_support);

    b = space.get_block(0, 8, 0);
    assert(b != 0);
    assert(b->yp == surface_none);


}
