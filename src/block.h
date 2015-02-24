#pragma once

enum block_type {
    block_empty,
    wall_support
};

enum surface_type {
    surface_none,
    surface_wall
};

/* a single block
 * represents a 1m^3 cube
 * for more information see docs/ships-space.md
 */
struct block {
    block_type type;

    /* 6 surfaces currently, all axis aligned
     * p stands for plus
     * m for minus
     *
     * so ym is the surface corresponding to the face
     *  on this block 'facing' down the +y axis
     */

    surface_type yp;
    surface_type ym;

    surface_type xp;
    surface_type xm;

    surface_type zp;
    surface_type zm;
};


