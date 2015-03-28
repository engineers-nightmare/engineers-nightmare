#include <stdio.h>
#include <assert.h>
#include "../src/ship_space.h"

void
simple(void)
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

void
test_mock_ship_space(void)
{
    unsigned int x=0, y=0, z=0;
    ship_space *ss = ship_space::mock_ship_space();
    assert(ss);

    block * b;

    for( z=0; z < 8; ++z ){
        for( y=0; y < 8; ++y ){
            for( x=0; x < 8; ++x ){

                b = ss->get_block(x, y, z);
                assert(b);
                if( z == 0 ){
                    assert(b->type == block_support);
                } else if( y == 0 ){
                    /* one wall */
                    assert(b->type == block_support);
                } else if( x == 0 ){
                    /* two wall */
                    assert(b->type == block_support);
                } else if( x == 7 ){
                    /* three wall */
                    assert(b->type == block_support);
                } else if( y == 7 ){
                    /* four wall */
                    assert(b->type == block_support);
                } else {
                    assert(b->type == block_empty);
                }


            }
        }
    }

}

/* some more quick and dirty 'testing'
 * mostly checking we compile and nothing
 * blows up obviously
 */
int
main(void)
{
    simple();
    test_mock_ship_space();
}
