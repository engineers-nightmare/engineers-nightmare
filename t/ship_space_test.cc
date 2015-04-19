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
test_mock_ship_space(void)
{
    unsigned int x=0, y=0, z=0;
    ship_space *ss = ship_space::mock_ship_space();
    assert(ss);

    block * b1 = 0;
    block * b2 = 0;
    block * b3 = 0;
    block * b4 = 0;

    for( z=0; z < 8; ++z ){
        for( y=0; y < 8; ++y ){
            for( x=0; x < 8; ++x ){

                b1 = ss->get_block(x,   y,   z);
                b3 = ss->get_block(x,   y+8, z);
                b2 = ss->get_block(x+8, y,   z);
                b4 = ss->get_block(x+8, y+8, z);

                assert(b1);
                assert(b2);
                assert(b3);
                assert(b4);

                if( z == 0 ){
                    assert(b1->type == block_support);
                    assert(b2->type == block_support);
                    assert(b3->type == block_support);
                    assert(b4->type == block_support);
                } else if( y == 0 ){
                    if( x == 3     && z == 1 ||
                        x == 3     && z == 2 ||
                        x == 5     && z == 1 ){
                        /* a door */
                        assert(b1->type == block_empty);
                        assert(b2->type == block_empty);
                        assert(b3->type == block_empty);
                        assert(b4->type == block_empty);
                    } else {
                        /* a wall */
                        assert(b1->type == block_support);
                        assert(b2->type == block_support);
                        assert(b3->type == block_support);
                        assert(b4->type == block_support);
                    }
                } else if( x == 0 ){
                    if( y == 3     && z == 1 ||
                        y == 3     && z == 2 ||
                        y == 5     && z == 1 ){
                        /* a door */
                        assert(b1->type == block_empty);
                        assert(b2->type == block_empty);
                        assert(b3->type == block_empty);
                        assert(b4->type == block_empty);
                    } else {
                        /* a wall */
                        assert(b1->type == block_support);
                        assert(b2->type == block_support);
                        assert(b3->type == block_support);
                        assert(b4->type == block_support);
                    }
                } else if( x == 7 ){
                    if( y == 3     && z == 1 ||
                        y == 3     && z == 2 ||
                        y == 5     && z == 1 ){
                        /* a door */
                        assert(b1->type == block_empty);
                        assert(b2->type == block_empty);
                        assert(b3->type == block_empty);
                        assert(b4->type == block_empty);
                    } else {
                        /* a wall */
                        assert(b1->type == block_support);
                        assert(b2->type == block_support);
                        assert(b3->type == block_support);
                        assert(b4->type == block_support);
                    }
                } else if( y == 7 ){
                    if( x == 3     && z == 1 ||
                        x == 3     && z == 2 ||
                        x == 5     && z == 1 ){
                        /* a door */
                        assert(b1->type == block_empty);
                        assert(b2->type == block_empty);
                        assert(b3->type == block_empty);
                        assert(b4->type == block_empty);
                    } else {
                        /* a wall */
                        assert(b1->type == block_support);
                        assert(b2->type == block_support);
                        assert(b3->type == block_support);
                        assert(b4->type == block_support);
                    }
                } else {
                    assert(b1->type == block_empty);
                    assert(b2->type == block_empty);
                    assert(b3->type == block_empty);
                    assert(b4->type == block_empty);
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
