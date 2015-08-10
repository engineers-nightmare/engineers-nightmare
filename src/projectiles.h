#pragma once
#include <glm/glm.hpp>

struct projectile
{
    glm::vec3 pos;
    glm::vec3 dir;
    float speed;
    float lifetime;

    /* Returns true if still alive */
    bool update(float dt);
};

void
update_projectiles(float dt);

void
draw_projectiles();

bool
spawn_projectile(glm::vec3 pos, glm::vec3 dir);
