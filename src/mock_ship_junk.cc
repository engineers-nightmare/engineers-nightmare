#include "ship_space.h"
#include <assert.h>
#include <math.h>
#include <algorithm>


/* given the x, y, z of a block and the surface we are interested in,
 * find the co-ords the correspond to the block along the normal of that surace
 *
 * tx, ty, and tz are out params
 */
static void
find_neighbor(int fx, int fy, int fz, enum surface_index si, int *tx, int *ty, int *tz){
    if( ! tx ||
        ! ty ||
        ! tz ){
        errx(1, "ship_space.c: find_neighbor tx, ty or tz null");
        return;
    }

    *tx = fx;
    *ty = fy;
    *tz = fz;

    switch( si ){
        case surface_xp:
            ++*tx;
            break;

        case surface_xm:
            --*tx;
            break;

        case surface_yp:
            ++*ty;
            break;

        case surface_ym:
            --*ty;
            break;

        case surface_zp:
            ++*tz;
            break;

        case surface_zm:
            --*tz;
            break;

        case face_count:
            errx(1, "ship_space.c: find_neighbor supplied surface_index of type 'face_count'");
            break;

        default:
            errx(1, "ship_space.c: find_neighbor supplied surface_index of unknown type");
            break;
    }
}

/* returns a block
 * finds the block at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 * will call ensure_block if needed
 */
static block *
ensure_and_get_block(ship_space *ss, glm::ivec3 bl) {
    block *b = 0;

    ss->ensure_block(bl);
    b = ss->get_block(bl);

    if( ! b ){
        errx(1, "ship_space::ensure_and_get_block: call to get_block failed");
    }

    return b;
}

/* returns the neighbor of a block along a given suface's normal
 * finds the block at the position (x,y,z) within
 * the whole ship_space
 * will move across chunks
 * will call ensure_block if needed
 */
static block *
ship_get_block_neighbor(ship_space * ss, glm::ivec3 block, enum surface_index si){
    glm::ivec3 t;
    find_neighbor(block.x, block.y, block.z, si, &t.x, &t.y, &t.z);
    return ensure_and_get_block(ss, t);
}

/* I am lazy
 * we plan to nuke mock ship space once we have map loading/saving
 *
 * Set Neighbour for block N
 */
#define sn1(si, type) (ship_get_block_neighbor(ss, \
            glm::ivec3(x + block_offsets[i][0],               \
            y + block_offsets[i][1],               \
            z), si)->surfs[si ^ 1] = type)

static int block_offsets[4][2] = { { 0, 0 }, { 0, 8 }, { 8, 0 }, { 8, 8 } };

/* returns a pointer to a new ship space
 * this ship space will have 2 x 2 rooms and will be 1 room tall
 * each room will have a floor and 4 walls of scaffolding
 * each room will have some doors on all 4 walls
 * there will be a floor of surfaces
 * and will otherwise be empty
 *
 * returns 0 on error
 */
ship_space *
ship_space::mock_ship_space(void)
{
    /* new ship space of 2 * 2 * 1*/
    ship_space * ss = new ship_space;
    for (unsigned x = 0; x < 2; x++) {
        for (unsigned y = 0; y < 2; y++) {
            for (unsigned z = 0; z < 1; z++) {
                ss->ensure_chunk(glm::ivec3(x, y, z));
            }
        }
    }

    /* LET THIS SERVE AS MOTIVATION FOR NEEDING MAP LOAD AND SAVE */

    /* first pass build complete outer shell for each chunk */
    for (unsigned z=0; z < 8; ++z) {
        for (unsigned y=0; y < 8; ++y) {
            for (unsigned x=0; x < 8; ++x) {
                for (int i = 0; i < 4; i++) {
                    block *b = ss->get_block(glm::ivec3(x + block_offsets[i][0], y + block_offsets[i][1], z));

                    if( z == 0 ){
                        /* the floor */
                        b->type = block_support;

                        /* add surfaces to inside */
                        b->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                        sn1(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));

                        /* add surfaces to outside */
                        b->surfs[surface_zm] = surface_wall;
                        sn1(surface_zm, surface_wall);

                    } else if( z == 7 ){
                        /* the roof */
                        b->type = block_support;

                        /* add surfaces to outside */
                        b->surfs[surface_zp] = surface_wall;
                        sn1(surface_zp, surface_wall);

                        /* add surfaces to inside */
                        b->surfs[surface_zm] = surface_wall;
                        sn1(surface_zm, surface_wall);

                    } else if( y == 0 || y == 7 ){
                        /* a wall */
                        b->type = block_support;

                        /* add surfaces to one side */
                        b->surfs[surface_yp] = surface_wall;
                        sn1(surface_yp, surface_wall);

                        /* add surfaces to other side */
                        b->surfs[surface_ym] = surface_wall;
                        sn1(surface_ym, surface_wall);

                    } else if( x == 0 || x == 7 ){
                        /* a wall */
                        b->type = block_support;

                        /* add surfaces to one side */
                        b->surfs[surface_xp] = surface_wall;
                        sn1(surface_xp, surface_wall);

                        /* add surfaces to other side */
                        b->surfs[surface_xm] = surface_wall;
                        sn1(surface_xm, surface_wall);

                    } else {
                        b->type = block_empty;
                    }
                }
            }
        }
    }

    return ss;
}
