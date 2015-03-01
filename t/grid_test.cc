#include <stdio.h>
#include <assert.h>
#include "../src/block.h"
#include "../src/grid.h"

/* thump around a bit hoping to catch
 * some low hanging fruit
 */
void
thump(void)
{
    grid_3d <block> grid_8(8, 8, 8);

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
test_extend_z_high(void)
{
    block * tmp;

    grid_3d <block> grid_8(8, 8, 8);

    assert( grid_8.get(0,0,0) != 0 );
    assert( grid_8.get(7,7,7) != 0 );

    assert( grid_8.get(8,7,7) == 0 );
    assert( grid_8.get(7,8,7) == 0 );
    assert( grid_8.get(7,7,8) == 0 );
    assert( grid_8.get(8,8,8) == 0 );

    /* mark some blocks in obvious ways */
    tmp = grid_8.get(0, 0, 0);
    assert( tmp != 0 );
    tmp->type = block_empty;

    tmp = grid_8.get(1, 1, 1);
    assert( tmp != 0 );
    tmp->type = block_support;


    /* grid_3d::extend returns 0 on success */
    assert( grid_8.extend(grid_8.extend_z, grid_8.extend_high, 2) == 0 );

    assert( grid_8.get(0,0,0) != 0 );
    assert( grid_8.get(7,7,7) != 0 );
    assert( grid_8.get(7,7,8) != 0 );
    assert( grid_8.get(7,7,9) != 0 );

    assert( grid_8.get(8,7,9) == 0 );
    assert( grid_8.get(7,8,9) == 0 );
    assert( grid_8.get(8,8,9) == 0 );
    assert( grid_8.get(7,7,10) == 0 );

    /* check that nothing has moved when it shouldnt */
    tmp = grid_8.get(0, 0, 0);
    assert( tmp != 0 );
    assert( tmp->type == block_empty );

    tmp = grid_8.get(1, 1, 1);
    assert( tmp != 0 );
    assert( tmp->type == block_support );
}

/* some light manual testing of block and grid
 */
int
main(void)
{
    thump();
    test_extend_z_high();
}

