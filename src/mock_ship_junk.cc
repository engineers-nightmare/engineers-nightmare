#include "ship_space.h"
#include <assert.h>
#include <math.h>
#include <algorithm>

/* I am lazy
 * we plan to nuke mock ship space once we have map loading/saving
 *
 * Set Neighbour for block N
 */
#define sn1(si, type) (ss->get_block_neighbor(     \
            glm::ivec3(x + block_offsets[i][0],    \
            y + block_offsets[i][1],               \
            z), si)->surfs[si ^ 1] = type)

static int block_offsets[4][2] = { { 0, 0 }, { 0, 8 }, { 8, 0 }, { 8, 8 } };

/* returns a pointer to a new ship space
 * this ship space will have 2 x 2 rooms and will be 1 room tall
 * each room will have a floor and 4 walls of framing
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
                        b->type = block_frame;

                        /* add surfaces to inside */
                        b->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                        sn1(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));

                        /* add surfaces to outside */
                        b->surfs[surface_zm] = surface_wall;
                        sn1(surface_zm, surface_wall);

                    } else if( z == 7 ){
                        /* the roof */
                        b->type = block_frame;

                        /* add surfaces to outside */
                        b->surfs[surface_zp] = surface_wall;
                        sn1(surface_zp, surface_wall);

                        /* add surfaces to inside */
                        b->surfs[surface_zm] = surface_wall;
                        sn1(surface_zm, surface_wall);

                    } else if( y == 0 || y == 7 ){
                        /* a wall */
                        b->type = block_frame;

                        /* add surfaces to one side */
                        b->surfs[surface_yp] = surface_wall;
                        sn1(surface_yp, surface_wall);

                        /* add surfaces to other side */
                        b->surfs[surface_ym] = surface_wall;
                        sn1(surface_ym, surface_wall);

                    } else if( x == 0 || x == 7 ){
                        /* a wall */
                        b->type = block_frame;

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
/* returns a pointer to a new ship space
 * this ship space will have 1 room and will be 3 units tall
 * this room will have a floor and 4 walls of framing
 * there will be a floor of surfaces
 * and will otherwise be empty
 *
 * returns 0 on error
 */
ship_space *
ship_space::mock_ship_space_2(void)
{
    /* new ship space of 1 * 1 * 1*/
    auto * ss = new ship_space;
    for (unsigned x = 0; x < 1; x++) {
        for (unsigned y = 0; y < 1; y++) {
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
                for (int i = 0; i < 1; i++) {
                    block *b = ss->get_block(glm::ivec3(x + block_offsets[i][0], y + block_offsets[i][1], z));

                    if( z == 0 ){
                        /* the floor */
                        b->type = block_frame;

                        /* add surfaces to inside */
                        b->surfs[surface_zp] = (x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall;
                        sn1(surface_zp, ((x >= 2 && x < 6 && y >= 2 && y < 6) ? surface_grate : surface_wall));

                        /* add surfaces to outside */
                        b->surfs[surface_zm] = surface_wall;
                        sn1(surface_zm, surface_wall);

                    } else if( z >= 4 ){
                        b->type = block_frame;

                        /* the roof */
                        if (z == 4) {
                            /* add surfaces to outside */
                            b->surfs[surface_zp] = surface_wall;
                            sn1(surface_zp, surface_wall);

                            /* add surfaces to inside */
                            b->surfs[surface_zm] = surface_wall;
                            sn1(surface_zm, surface_wall);
                        }

                    } else if( y == 0 || y == 7 ){
                        /* a wall */
                        b->type = block_frame;

                        /* add surfaces to one side */
                        b->surfs[surface_yp] = surface_wall;
                        sn1(surface_yp, surface_wall);

                        /* add surfaces to other side */
                        b->surfs[surface_ym] = surface_wall;
                        sn1(surface_ym, surface_wall);

                    } else if( x == 0 || x == 7 ){
                        /* a wall */
                        b->type = block_frame;

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
