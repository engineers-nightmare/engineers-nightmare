#include <stdio.h>
#include <assert.h>
#include "../src/block.h"
#include "../src/grid.h"

/* some light manual testing of block and grid
 */

int
main(void)
{
    grid_3d <block, 8> grid_8;

    /* just some gentle testing to make sure nothing
     * obviously broken happens
     */
    for( int i = 0; i<8; ++i ){
        for( int j = 0; j<8; ++j ){
            for( int k = 0; k<8; ++k ){
                grid_8.contents[i][j][k].type = wall_support;
                switch(k){
                    case 0:
                        grid_8.contents[i][j][k].yp = surface_wall;
                        grid_8.contents[i][j][k].ym = surface_none;
                        break;
                    case 1:
                        grid_8.contents[i][j][k].yp = surface_none;
                        grid_8.contents[i][j][k].ym = surface_wall;
                        break;
                    case 2:
                        grid_8.contents[i][j][k].xp = surface_wall;
                        grid_8.contents[i][j][k].xm = surface_none;
                        break;
                    case 3:
                        grid_8.contents[i][j][k].xp = surface_none;
                        grid_8.contents[i][j][k].xm = surface_wall;
                        break;
                    case 4:
                        grid_8.contents[i][j][k].zp = surface_wall;
                        grid_8.contents[i][j][k].zm = surface_none;
                        break;
                    case 6:
                        grid_8.contents[i][j][k].zp = surface_none;
                        grid_8.contents[i][j][k].zm = surface_wall;
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
                assert( grid_8.contents[i][j][k].type == wall_support );
                switch(k){
                    case 0:
                        assert( grid_8.contents[i][j][k].yp == surface_wall );
                        assert( grid_8.contents[i][j][k].ym == surface_none );
                        break;
                    case 1:
                        assert( grid_8.contents[i][j][k].yp == surface_none );
                        assert( grid_8.contents[i][j][k].ym == surface_wall );
                        break;
                    case 2:
                        assert( grid_8.contents[i][j][k].xp == surface_wall );
                        assert( grid_8.contents[i][j][k].xm == surface_none );
                        break;
                    case 3:
                        assert( grid_8.contents[i][j][k].xp == surface_none );
                        assert( grid_8.contents[i][j][k].xm == surface_wall );
                        break;
                    case 4:
                        assert( grid_8.contents[i][j][k].zp == surface_wall );
                        assert( grid_8.contents[i][j][k].zm == surface_none );
                        break;
                    case 6:
                        assert( grid_8.contents[i][j][k].zp == surface_none );
                        assert( grid_8.contents[i][j][k].zm == surface_wall );
                        break;
                    case 7:
                    default:
                        break;
                }
            }
        }
    }


}
