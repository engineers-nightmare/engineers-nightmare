#include "ship_space.h"
#include <assert.h>
#include <math.h>
#include <algorithm>

ship_space *
ship_space::mock_ship_space()
{
    ship_space * ss = new ship_space;
    ss->ensure_chunk(glm::ivec3(0, 0, 0));
    ss->rebuild_topology();
    ss->cut_out_cuboid(glm::ivec3(1, 1, 1), glm::ivec3(5, 5, 3), surface_wall);
    ss->get_block(glm::ivec3(1, 1, 1))->type = block_invcorner_base;
    ss->get_block(glm::ivec3(2, 1, 1))->type = block_corner_base;
    ss->get_block(glm::ivec3(1, 2, 1))->type = block_corner_base;
    ss->get_block(glm::ivec3(1, 1, 2))->type = block_corner_base;
    ss->get_block(glm::ivec3(5, 1, 1))->type = (block_type)(block_corner_base + block_bit_xp);
    ss->get_block(glm::ivec3(1, 5, 1))->type = (block_type)(block_corner_base + block_bit_yp);
    ss->get_block(glm::ivec3(1, 1, 3))->type = (block_type)(block_corner_base + block_bit_zp);
    ss->get_block(glm::ivec3(5, 5, 1))->type = (block_type)(block_corner_base + block_bit_xp + block_bit_yp);
    ss->get_block(glm::ivec3(5, 1, 3))->type = (block_type)(block_corner_base + block_bit_xp + block_bit_zp);
    ss->get_block(glm::ivec3(1, 5, 3))->type = (block_type)(block_corner_base + block_bit_yp + block_bit_zp);
    ss->get_block(glm::ivec3(5, 5, 3))->type = (block_type)(block_corner_base + block_bit_xp + block_bit_yp + block_bit_zp);

    return ss;
}

