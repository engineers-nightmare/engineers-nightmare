#pragma once

#include "wiring/wiring_data.h"

enum block_type {
    block_untouched,
    block_empty,
    block_frame,
    block_corner_base = 8,
    block_invcorner_base = 16,
    block_slope_base = 24,
    block_slope_extra_base = 32,

    // bits to be combined with block_*_base for variants.
    block_bit_xp = 1,
    block_bit_yp = 2,
    block_bit_zp = 4,
};

enum surface_type : unsigned char {
    surface_phys = 0x20,
    surface_blocks_light = 0x40,
    surface_blocks_air = 0x80,
    surface_none = 0,

    surface_wall = surface_phys | surface_blocks_light | surface_blocks_air,

    surface_door = surface_blocks_light | surface_blocks_air,

    surface_grate = surface_phys | surface_blocks_light,

    surface_glass = surface_phys | surface_blocks_air,
};

/* 6 surfaces currently, all axis aligned
 * p stands for plus
 * m for minus
 *
 * so ym is the surface corresponding to the face
 *  on this block 'facing' down the +y axis
 */
enum surface_index : unsigned {
    surface_xp,
    surface_xm,
    surface_yp,
    surface_ym,
    surface_zp,
    surface_zm,

    face_count,
};

struct block_wires {
    bool has_wire[face_count]{};
    unsigned wire_bits[face_count]{};
};

/* a single block
 * represents a 1m^3 cube
 * for more information see docs/ships-space.md
 */
struct block {
    block_type type = block_untouched;

    surface_type surfs[face_count]{};
    block_wires wire[num_wire_types];
};

static inline unsigned char  /* bool */
air_permeable(surface_type s)
{
    return ~s & surface_blocks_air;
}
