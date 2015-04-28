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

                    b1->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    b2->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    b3->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                    b4->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;

                } else if( y == 0 ){
                    if( (x == 3     && z == 1) ||
                        (x == 3     && z == 2) ||
                        (x == 5     && z == 1) ){
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
                    if( (y == 3     && z == 1) ||
                        (y == 3     && z == 2) ||
                        (y == 5     && z == 1) ){
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
                    if( (y == 3     && z == 1) ||
                        (y == 3     && z == 2) ||
                        (y == 5     && z == 1) ){
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
                    if( (x == 3     && z == 1) ||
                        (x == 3     && z == 2) ||
                        (x == 5     && z == 1) ){
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

void
ensure(void)
{
    ship_space space(1, 1, 1);

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
    assert( 0 == space.get_block(0,0,-8) );
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
    test_mock_ship_space();
    ensure();
}
