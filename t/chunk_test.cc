#include <stdio.h>
#include <assert.h>
#include "../src/chunk.h"

/* thump around a bit hoping to catch
 * some low hanging fruit
 */
void
thump(void)
{
    chunk c;

    /* just some gentle testing to make sure nothing
     * obviously broken happens
     */
    for( int i = 0; i<8; ++i ){
        for( int j = 0; j<8; ++j ){
            for( int k = 0; k<8; ++k ){
                c.get_block(i, j, k)->type = block_support;
                switch(k){
                    case 0:
                        c.get_block(i, j, k)->yp = surface_wall;
                        c.get_block(i, j, k)->ym = surface_none;
                        break;
                    case 1:
                        c.get_block(i, j, k)->yp = surface_none;
                        c.get_block(i, j, k)->ym = surface_wall;
                        break;
                    case 2:
                        c.get_block(i, j, k)->xp = surface_wall;
                        c.get_block(i, j, k)->xm = surface_none;
                        break;
                    case 3:
                        c.get_block(i, j, k)->xp = surface_none;
                        c.get_block(i, j, k)->xm = surface_wall;
                        break;
                    case 4:
                        c.get_block(i, j, k)->zp = surface_wall;
                        c.get_block(i, j, k)->zm = surface_none;
                        break;
                    case 6:
                        c.get_block(i, j, k)->zp = surface_none;
                        c.get_block(i, j, k)->zm = surface_wall;
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
                assert( c.get_block(i, j, k)->type == block_support );
                switch(k){
                    case 0:
                        assert( c.get_block(i, j, k)->yp == surface_wall );
                        assert( c.get_block(i, j, k)->ym == surface_none );
                        break;
                    case 1:
                        assert( c.get_block(i, j, k)->yp == surface_none );
                        assert( c.get_block(i, j, k)->ym == surface_wall );
                        break;
                    case 2:
                        assert( c.get_block(i, j, k)->xp == surface_wall );
                        assert( c.get_block(i, j, k)->xm == surface_none );
                        break;
                    case 3:
                        assert( c.get_block(i, j, k)->xp == surface_none );
                        assert( c.get_block(i, j, k)->xm == surface_wall );
                        break;
                    case 4:
                        assert( c.get_block(i, j, k)->zp == surface_wall );
                        assert( c.get_block(i, j, k)->zm == surface_none );
                        break;
                    case 6:
                        assert( c.get_block(i, j, k)->zp == surface_none );
                        assert( c.get_block(i, j, k)->zm == surface_wall );
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

