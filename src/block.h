#pragma once

enum block_type {
    block_empty,
    block_support,
    block_entity,
};

enum surface_type {
    surface_blocks_light = 0x40,
    surface_blocks_air = 0x80,
    surface_none = 0,

    surface_wall = surface_blocks_light | surface_blocks_air,

    surface_grate = surface_blocks_light,

    surface_glass = surface_blocks_air,
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

    face_count,
};

/* a single block
 * represents a 1m^3 cube
 * for more information see docs/ships-space.md
 */
struct block {
    block_type type;

    surface_type surfs[face_count];
    unsigned short surf_space[face_count];
};

static inline bool
air_permeable(surface_type s)
{
    return ~s & surface_blocks_air;
}

static inline bool
light_permeable(surface_type s)
{
    return ~s & surface_blocks_light;
}
