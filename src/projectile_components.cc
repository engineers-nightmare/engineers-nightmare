#include "projectile_components.h"
#include "physics.h"
#include <algorithm>
#include "mesh.h"
#include "shader_params.h"

extern physics *phy;
extern hw_mesh *projectile_hw;

extern glm::mat4
mat_position(glm::vec3);

#define MAX_PROJECTILES 2000

void
projectile_manager::create_component_instance_data(unsigned count) {
    if (count <= projectile_pool.allocated)
        return;

    projectile_instance_data data;

    auto entity_size = sizeof(c_entity) * count;

    auto mass_size = sizeof(float) * count;
    mass_size += mass_size % alignof(float);

    auto lifetime_size = sizeof(float) * count;
    lifetime_size += lifetime_size % alignof(float);

    auto position_size = sizeof(glm::vec3) * count;
    position_size += position_size % alignof(glm::vec3);

    auto velocity_size = sizeof(glm::vec3) * count;
    velocity_size += velocity_size % alignof(glm::vec3);

    const auto bytes = entity_size + mass_size + lifetime_size + position_size + velocity_size;

    data.buffer = malloc(bytes);
    data.num = projectile_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.mass = (float *)(data.entity + count);
    data.mass = (float*)((size_t)data.mass + alignof(float)-1 & ~(alignof(float)-1));

    data.lifetime = (float *)(data.mass + count);
    data.lifetime = (float*)((size_t)data.lifetime + alignof(float)-1 & ~(alignof(float)-1));

    data.position = (glm::vec3 *)(data.lifetime + count);
    data.position = (glm::vec3*)((size_t)data.position + alignof(glm::vec3) - 1 & ~(alignof(glm::vec3) - 1));

    data.velocity = data.position + count;
    data.velocity = (glm::vec3*)((size_t)data.velocity + alignof(glm::vec3) - 1 & ~(alignof(glm::vec3) - 1));

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
        // lazy allocate more than we need with room to spare
        auto count = std::max(projectile_pool.num, (unsigned)MAX_PROJECTILES);
        create_component_instance_data(count * 2);
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
projectile_manager::destroy_instance(instance i) {
    auto last_id = projectile_pool.num - 1;
    auto last = projectile_pool.entity[last_id];

    projectile_pool.entity[i.index] = projectile_pool.entity[last_id];
    projectile_pool.mass[i.index] = projectile_pool.mass[last_id];
    projectile_pool.lifetime[i.index] = projectile_pool.lifetime[last_id];
    projectile_pool.position[i.index] = projectile_pool.position[last_id];
    projectile_pool.velocity[i.index] = projectile_pool.velocity[last_id];

    entity_instance_map[last] = i.index;
    entity_instance_map.erase(last);

    --projectile_pool.num;
}

void projectile_linear_manager::simulate(float dt) {
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
            destroy_instance(inst);
            --i;
        }

        ++i;
    }
}

void projectile_sine_manager::simulate(float dt) {
    static auto time = 0.f;
    time += dt;
    for (auto i = 0u; i < projectile_pool.num; ) {
        auto inst = make_instance(i);
        if (velocity(inst) != glm::vec3(0)) {
            auto new_pos = position(inst) + velocity(inst) * dt;
            new_pos.z += sin(lifetime(inst) * 20)* 0.01f;

            auto hit = phys_raycast_generic(position(inst), new_pos,
                phy->ghostObj, phy->dynamicsWorld);

            if (hit.hit) {
                new_pos = hit.hitCoord;
                set_velocity(inst, glm::vec3(0));
                set_lifetime(inst, after_collision_lifetime);
            }

            set_position(inst, new_pos);
        }

        set_lifetime(inst, lifetime(inst) - dt);

        if (lifetime(inst) <= 0.f) {
            destroy_instance(inst);
            --i;
        }

        ++i;
    }
}