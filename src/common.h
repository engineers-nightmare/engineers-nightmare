#pragma once

#include <glm/glm.hpp>

#ifndef _WIN32
#define __unused __attribute__(( unused ))
#else
#define __unused
#endif // _WIN32

#define PI 3.14159265f
#define DEG2RAD(x) (x * PI / 180.f)
#define RAD2DEG(x) (x * 180.f / PI)


inline glm::ivec3
get_coord_containing(glm::vec3 v) {
    /* truncating via int cast */
    int x = (int)v.x; if (v.x < 0) x--;
    int y = (int)v.y; if (v.y < 0) y--;
    int z = (int)v.z; if (v.z < 0) z--;

    return glm::ivec3(x, y, z);
}
