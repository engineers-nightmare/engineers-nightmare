#pragma once

enum block_type {
    block_empty,
    block_support,
    block_entity,
};

enum surface_type {
    surface_none,
    surface_wall,
    surface_grate,
    surface_text,
};

/* 6 surfaces currently, all axis aligned
 * p stands for plus
 * m for minus
 *
 * so ym is the surface corresponding to the face
 *  on this block 'facing' down the +y axis
 */
enum surface_index {
    surface_xp,
    surface_xm,
    surface_yp,
    surface_ym,
    surface_zp,
    surface_zm,
};

/* a single block
 * represents a 1m^3 cube
 * for more information see docs/ships-space.md
 */
struct block {
    block_type type;

    surface_type surfs[6];

    /* manually set everything to default state
     * do not rely on any default behavior
     */
    block(void) :
        type(block_empty)
    {}
};


