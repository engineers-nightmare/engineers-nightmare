#pragma once

#include <glm/glm.hpp>

struct player {
    float angle;
    float elev;
    glm::vec3 pos;
    glm::vec3 dir;  /* computed */
    glm::vec2 move;

    bool last_jump;
    bool jump;

    bool scan;

    bool last_place;
    bool place;
};

