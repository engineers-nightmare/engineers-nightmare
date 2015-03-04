#include <stdio.h>
#include <assert.h>
#include "../src/block.h"
#include "../src/fixed_grid.h"

/* thump around a bit hoping to catch
 * some low hanging fruit
 */
void
thump(void)
{
    fixed_grid <block> grid_8(8, 8, 8);

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

    /* FIXME moving extend logic out of grid space */
    return;

    fixed_grid <block> grid_8(8, 8, 8);

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

    /* fixed_grid::extend returns 0 on success */
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

void
test_extend_z_low(void)
{
    int i, j, k;

    block * tmp;

    /* FIXME moving extend logic out of grid space */
    return;

    fixed_grid <block> grid_8(8, 8, 8);

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

    /* exhaustively check things are as they should be
     * NB: this relies on block_empty being the DEFAULT, see block::block
     */
    for( i=0; i<8; ++i ){
        for( j=0; j<8; ++j ){
            for( k=0; k<8; ++k ){
                tmp = grid_8.get(i, j, k);
                assert( tmp != 0 );
                if( i==1 && j ==1 && k==1 ){
                    assert( tmp->type == block_support );
                } else {
                    assert( tmp->type == block_empty );
                }
            }
        }
    }

    /* fixed_grid::extend returns 0 on success */
    assert( grid_8.extend(grid_8.extend_z, grid_8.extend_low, 2) == 0 );

    assert( grid_8.get(0,0,0) != 0 );
    assert( grid_8.get(7,7,7) != 0 );
    assert( grid_8.get(7,7,8) != 0 );
    assert( grid_8.get(7,7,9) != 0 );

    assert( grid_8.get(8,7,9) == 0 );
    assert( grid_8.get(7,8,9) == 0 );
    assert( grid_8.get(8,8,9) == 0 );
    assert( grid_8.get(7,7,10) == 0 );

    /* check that things have moved as they should have
     * NB: z is now 2 further along (as 2 was our extend_by to extend)
     */
    tmp = grid_8.get(0, 0, 2);
    assert( tmp != 0 );
    assert( tmp->type == block_empty );

    tmp = grid_8.get(1, 1, 3);
    assert( tmp != 0 );
    assert( tmp->type == block_support );

    /* exhaustively check things are as they should be
     * NB: this relies on block_empty being the DEFAULT, see block::block
     */
    for( i=0; i<8; ++i ){
        for( j=0; j<8; ++j ){
            for( k=0; k<10; ++k ){
                tmp = grid_8.get(i, j, k);
                assert( tmp != 0 );

                if( i==1 && j ==1 && k==3 ){
                    assert( tmp->type == block_support );
                } else {
                    assert( tmp->type == block_empty );
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
    thump();
}

