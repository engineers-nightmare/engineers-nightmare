#include <stdio.h>
#include <assert.h>
#include "../src/block.h"
#include "../src/dynamic_grid.h"


void
test_negative(void)
{
    int x=0, y=0, z=0;
    dynamic_grid<int> *dg = new dynamic_grid<int>(-10, 0, -10, 0, -10, 0);
    assert(dg);

    /* test inside bounds */
    for( z=-10; z<0; ++z ){
        for( y=-10; y<0; ++y ){
            for( x=-10; x<0; ++x ){
                assert(dg->get(x, y, z));
            }
        }
    }

    /* test negative out of bounds */
    for( z=-20; z<-10; ++z ){
        for( y=-20; y<-10; ++y ){
            for( x=-20; x<-10; ++x ){
                assert(dg->get(x, y, z) == 0);
            }
        }
    }

    /* test positive out of bounds */
    for( z=0; z<10; ++z ){
        for( y=0; y<10; ++y ){
            for( x=0; x<10; ++x ){
                assert(dg->get(x, y, z) == 0);
            }
        }
    }


}

void test_expand(void)
{
    int x=0, y=0, z=0;
    dynamic_grid<int> *dg = new dynamic_grid<int>(2, 2, 2);
    int *i = 0;
    assert(dg);


    /* set the stage */
    for( z=0; z<2; ++z ){
        for( y=0; y<2; ++y ){
            for( x=0; x<2; ++x ){
                *dg->get(x, y, z) = x * y * z;
            }
        }
    }

    /* force a resize */
    dg->ensure(2, 3, 4);

    /* check everything is good */
    for( z=0; z<4; ++z ){
        for( y=0; y<3; ++y ){
            for( x=0; x<2; ++x ){
                i = dg->get(x, y, z);
                assert(i);

                if( z < 2 && y < 2 && x < 2 ){
                    assert( *i == x * y * z );
                } else {
                    assert( *i == 0 );
                }
            }
        }
    }
}

/* some light manual testing of block and grid
 */
int
main(void)
{
    test_negative();
    test_expand();
}

