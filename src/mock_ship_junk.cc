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

    return ss;
}

