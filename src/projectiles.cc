#include <epoxy/gl.h>

#include "projectiles.h"
#include "mesh.h"
#include "player.h"
#include "physics.h"
#include "shader_params.h"
#include "component/component.h"
#include <epoxy/gl.h>

#define MAX_PROJECTILES 200
#define PROJECTILE_INITIAL_SPEED    2.f
#define PROJECTILE_INITIAL_LIFETIME 10.f
#define PROJECTILE_AFTER_COLLISION_LIFETIME 0.5f

//extern shader_params<per_object_params> *per_object;
//extern hw_mesh *projectile_hw;
//extern physics *phy;

//extern glm::mat4
//mat_position(glm::vec3 pos);

projectile_manager proj_man(MAX_PROJECTILES);

bool
projectile::update(float dt)
{
    //auto new_pos = pos + dir * speed * dt;

    //auto hit = phys_raycast_generic(pos, new_pos,
    //        phy->ghostObj, phy->dynamicsWorld);

    //if (hit.hit) {
    //    new_pos = hit.hitCoord;
    //    speed = 0.f;
    //    lifetime = PROJECTILE_AFTER_COLLISION_LIFETIME;
    //}

    //pos = new_pos;

    //lifetime -= dt;
    //return lifetime > 0.f;
    return true;
}

//projectile *projectiles = new projectile[MAX_PROJECTILES];
//unsigned num_projectiles = 0;

void
update_projectiles(float dt)
{
    ///* update the projectiles */
    //for (auto i = 0u; i < num_projectiles;) {
    //    if (projectiles[i].update(dt)) {
    //        ++i;
    //    }
    //    else {
    //        projectiles[i] = projectiles[--num_projectiles];
    //    }
    //}
    proj_man.simulate(dt);
}

void
draw_projectiles()
{
    //for (auto i = 0u; i < num_projectiles; i++) {
    //    /* TODO: instancing etc to get rid of this upload/draw loop */
    //    per_object->val.world_matrix = mat_position(projectiles[i].pos);
    //    per_object->upload();
    //    draw_mesh(projectile_hw);
    //}
    proj_man.draw();
}

bool
spawn_projectile(glm::vec3 pos, glm::vec3 dir)
{
    //if (num_projectiles >= MAX_PROJECTILES) {
    //    return false;
    //}

    //auto & proj = projectiles[num_projectiles++];
    //proj.pos = pos;
    //proj.dir = dir;
    //proj.speed = PROJECTILE_INITIAL_SPEED;
    //proj.lifetime = PROJECTILE_INITIAL_LIFETIME;
    //return true;
    proj_man.spawn(pos, dir * PROJECTILE_INITIAL_SPEED);
    return true;
}
