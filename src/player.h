#pragma once

#include <glm/glm.hpp>

#define EYE_OFFSET_Z    0.3

struct player {
    float angle;
    float elev;
    glm::vec3 pos;
    glm::vec3 dir;  /* computed */
    glm::vec2 move;

    int selected_slot;

    bool last_jump;
    bool jump;

    bool last_use;
    bool use;

    bool last_reset;
    bool reset;
};

