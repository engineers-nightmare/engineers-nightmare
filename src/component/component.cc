#include <stdlib.h>
#include <string.h>

#include "component.h"

#include "../physics.h"
#include "../shader_params.h"
#include "../mesh.h"

extern physics *phy;
extern hw_mesh *projectile_hw;

extern glm::mat4
mat_position(glm::vec3);

projectile_manager::projectile_manager(unsigned count) {
    if (count > 0) {
        create_projectile_instance_data(count);
    }
}


void
projectile_manager::create_projectile_instance_data(unsigned count) {
    if (count <= projectile_pool.allocated)
        return;

    projectile_instance_data data;
    const auto bytes = count * (sizeof(c_entity) + sizeof(float) + 2 * sizeof(glm::vec3));
    data.buffer = malloc(bytes);
    data.num = projectile_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;
    data.mass = (float *)(data.entity + count);
    data.lifetime = (float *)(data.entity + count);
    data.position = (glm::vec3 *)(data.mass + count);
    data.velocity = data.position + count;

    memcpy(data.entity, projectile_pool.entity, projectile_pool.num * sizeof(c_entity));
    memcpy(data.mass, projectile_pool.mass, projectile_pool.num * sizeof(float));
    memcpy(data.lifetime, projectile_pool.lifetime, projectile_pool.num * sizeof(float));
    memcpy(data.position, projectile_pool.position, projectile_pool.num * sizeof(glm::vec3));
    memcpy(data.velocity, projectile_pool.velocity, projectile_pool.num * sizeof(glm::vec3));

    free(projectile_pool.buffer);
    projectile_pool = data;
}

void
projectile_manager::spawn(glm::vec3 pos, glm::vec3 vel) {
    if (projectile_pool.num >= projectile_pool.allocated) {
        return;
    }

    auto i = make_instance(projectile_pool.num);

    set_position(i, pos);
    set_velocity(i, vel);
    set_lifetime(i, initial_lifetime);

    ++projectile_pool.num;
}

void
projectile_manager::draw() {
    for (auto i = 0u; i < projectile_pool.num; ++i) {
        auto inst = make_instance(i);
        /* TODO: instancing etc to get rid of this upload/draw loop */
        per_object->val.world_matrix = mat_position(position(inst));
        per_object->upload();
        draw_mesh(projectile_hw);
    }
}

void
projectile_manager::simulate(float dt) {
    for (auto i = 0u; i < projectile_pool.num; ) {
        auto inst = make_instance(i);
        auto new_pos = position(inst) + velocity(inst) * dt;

        auto hit = phys_raycast_generic(position(inst), new_pos,
            phy->ghostObj, phy->dynamicsWorld);

        if (hit.hit) {
            new_pos = hit.hitCoord;
            set_velocity(inst, glm::vec3(0));
            set_lifetime(inst, after_collision_lifetime);
        }

        set_position(inst, new_pos);

        set_lifetime(inst, lifetime(inst) - dt);

        if (lifetime(inst) <= 0.f) {
            destroy(inst);
            --i;
        }

        ++i;
    }
}

void
projectile_manager::destroy(instance i) {
    auto last_id = projectile_pool.num - 1;
    auto e = projectile_pool.entity[i.i];
    auto last = projectile_pool.entity[last_id];

    projectile_pool.entity[i.i] = projectile_pool.entity[last_id];
    projectile_pool.mass[i.i] = projectile_pool.mass[last_id];
    projectile_pool.lifetime[i.i] = projectile_pool.lifetime[last_id];
    projectile_pool.position[i.i] = projectile_pool.position[last_id];
    projectile_pool.velocity[i.i] = projectile_pool.velocity[last_id];

    map[last.id] = i.i;
    map.erase(i.i);

    --projectile_pool.num;
}