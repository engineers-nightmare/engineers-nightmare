#pragma once

#include <glm/glm.hpp>

#define PLAYER_RADIUS          0.23f
#define PLAYER_STAND_HEIGHT    1.5f
#define PLAYER_CROUCH_HEIGHT   0.9f

/* eye offset is down from top of player capsule (height) */
#define EYE_OFFSET_Z    0.1f

struct player {
    float angle;
    float elev;
    glm::vec3 pos;
    glm::vec3 dir;  /* computed */
    glm::vec3 eye;  /* computed */
    glm::vec2 move;

    float height;   /* filled by physics */

    unsigned int active_tool_slot;

    bool jump;

    bool use;

    bool cycle_mode;

    bool reset;

    bool crouch;
    bool crouch_end;

    bool gravity;

    bool use_tool;
    bool alt_use_tool;    
    bool long_use_tool;

    bool ui_dirty;

    /* used within physics to toggle gravity */
    bool disable_gravity;

    bool fire_projectile;
};

