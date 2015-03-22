#include <stdio.h>
#include <assert.h>
#include "../src/block.h"
#include "../src/dynamic_grid.h"

/* thump around a bit hoping to catch
 * some low hanging fruit
 */
void
thump(void)
{
    dynamic_grid <block> grid_8(8, 8, 8);

    /* just some gentle testing to make sure nothing
     * obviously broken happens
     */
    for( int i = 0; i<8; ++i ){
        for( int j = 0; j<8; ++j ){
            for( int k = 0; k<8; ++k ){
                grid_8.get(i, j, k)->type = block_support;
                switch(k){
                    case 0:
                        grid_8.get(i, j, k)->yp = surface_wall;
                        grid_8.get(i, j, k)->ym = surface_none;
                        break;
                    case 1:
                        grid_8.get(i, j, k)->yp = surface_none;
                        grid_8.get(i, j, k)->ym = surface_wall;
                        break;
                    case 2:
                        grid_8.get(i, j, k)->xp = surface_wall;
                        grid_8.get(i, j, k)->xm = surface_none;
                        break;
                    case 3:
                        grid_8.get(i, j, k)->xp = surface_none;
                        grid_8.get(i, j, k)->xm = surface_wall;
                        break;
                    case 4:
                        grid_8.get(i, j, k)->zp = surface_wall;
                        grid_8.get(i, j, k)->zm = surface_none;
                        break;
                    case 6:
                        grid_8.get(i, j, k)->zp = surface_none;
                        grid_8.get(i, j, k)->zm = surface_wall;
                        break;
                    case 7:
                    default:
                        break;
                }
            }
        }
    }

    for( int i = 0; i<8; ++i ){
        for( int j = 0; j<8; ++j ){
            for( int k = 0; k<8; ++k ){
                assert( grid_8.get(i, j, k)->type == block_support );
                switch(k){
                    case 0:
                        assert( grid_8.get(i, j, k)->yp == surface_wall );
                        assert( grid_8.get(i, j, k)->ym == surface_none );
                        break;
                    case 1:
                        assert( grid_8.get(i, j, k)->yp == surface_none );
                        assert( grid_8.get(i, j, k)->ym == surface_wall );
                        break;
                    case 2:
                        assert( grid_8.get(i, j, k)->xp == surface_wall );
                        assert( grid_8.get(i, j, k)->xm == surface_none );
                        break;
                    case 3:
                        assert( grid_8.get(i, j, k)->xp == surface_none );
                        assert( grid_8.get(i, j, k)->xm == surface_wall );
                        break;
                    case 4:
                        assert( grid_8.get(i, j, k)->zp == surface_wall );
                        assert( grid_8.get(i, j, k)->zm == surface_none );
                        break;
                    case 6:
                        assert( grid_8.get(i, j, k)->zp == surface_none );
                        assert( grid_8.get(i, j, k)->zm == surface_wall );
                        break;
                    case 7:
                    default:
                        break;
                }
            }
        }
    }
}

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

/* some light manual testing of block and grid
 */
int
main(void)
{
    thump();
    test_negative();
}

