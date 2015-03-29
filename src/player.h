#pragma once

#include <glm/glm.hpp>

struct player {
    float angle;
    float elev;
    glm::vec3 pos;
    glm::vec3 dir;  /* computed */
};

