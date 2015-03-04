#include <stdio.h>
#include <assert.h>
#include "../src/block.h"
#include "../src/fixed_cube.h"

/* thump around a bit hoping to catch
 * some low hanging fruit
 */
void
thump(void)
{
    fixed_cube <block, 8> cube_8;

    /* just some gentle testing to make sure nothing
     * obviously broken happens
     */
    for( int i = 0; i<8; ++i ){
        for( int j = 0; j<8; ++j ){
            for( int k = 0; k<8; ++k ){
                cube_8.get(i, j, k)->type = block_support;
                switch(k){
                    case 0:
                        cube_8.get(i, j, k)->yp = surface_wall;
                        cube_8.get(i, j, k)->ym = surface_none;
                        break;
                    case 1:
                        cube_8.get(i, j, k)->yp = surface_none;
                        cube_8.get(i, j, k)->ym = surface_wall;
                        break;
                    case 2:
                        cube_8.get(i, j, k)->xp = surface_wall;
                        cube_8.get(i, j, k)->xm = surface_none;
                        break;
                    case 3:
                        cube_8.get(i, j, k)->xp = surface_none;
                        cube_8.get(i, j, k)->xm = surface_wall;
                        break;
                    case 4:
                        cube_8.get(i, j, k)->zp = surface_wall;
                        cube_8.get(i, j, k)->zm = surface_none;
                        break;
                    case 6:
                        cube_8.get(i, j, k)->zp = surface_none;
                        cube_8.get(i, j, k)->zm = surface_wall;
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
                assert( cube_8.get(i, j, k)->type == block_support );
                switch(k){
                    case 0:
                        assert( cube_8.get(i, j, k)->yp == surface_wall );
                        assert( cube_8.get(i, j, k)->ym == surface_none );
                        break;
                    case 1:
                        assert( cube_8.get(i, j, k)->yp == surface_none );
                        assert( cube_8.get(i, j, k)->ym == surface_wall );
                        break;
                    case 2:
                        assert( cube_8.get(i, j, k)->xp == surface_wall );
                        assert( cube_8.get(i, j, k)->xm == surface_none );
                        break;
                    case 3:
                        assert( cube_8.get(i, j, k)->xp == surface_none );
                        assert( cube_8.get(i, j, k)->xm == surface_wall );
                        break;
                    case 4:
                        assert( cube_8.get(i, j, k)->zp == surface_wall );
                        assert( cube_8.get(i, j, k)->zm == surface_none );
                        break;
                    case 6:
                        assert( cube_8.get(i, j, k)->zp == surface_none );
                        assert( cube_8.get(i, j, k)->zm == surface_wall );
                        break;
                    case 7:
                    default:
                        break;
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

