#include "projectiles.h"
#include "component/component.h"

#define PROJECTILE_INITIAL_SPEED    2.f
#define PROJECTILE_INITIAL_LIFETIME 10.f
#define PROJECTILE_AFTER_COLLISION_LIFETIME 0.5f

projectile_sine_manager proj_man;

void
update_projectiles(float dt)
{
    proj_man.simulate(dt);
}

void
draw_projectiles()
{
    proj_man.draw();
}

bool
spawn_projectile(glm::vec3 pos, glm::vec3 dir)
{
    proj_man.spawn(pos, dir * PROJECTILE_INITIAL_SPEED);
    return true;
}
