#include <algorithm>
#include <assert.h>

#include "../common.h"
#include "projectile.h"
#include "../memory.h"
#include "../mesh.h"
#include "../physics.h"

extern physics *phy;

hw_mesh *projectile_hw;
sw_mesh *projectile_sw;

void
projectile_manager::create_projectile_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    projectile_instance_data new_pool;

    size_t size = sizeof(float) * count;
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);
    size += alignof(glm::vec3);     // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.mass = align_ptr((float *)new_buffer.buffer);
    new_pool.lifetime = align_ptr((float *)(new_pool.mass + count));
    new_pool.position = align_ptr((glm::vec3 *)(new_pool.lifetime + count));
    new_pool.velocity = align_ptr((glm::vec3 *)(new_pool.position + count));

    memcpy(new_pool.mass, projectile_pool.mass, buffer.num * sizeof(float));
    memcpy(new_pool.lifetime, projectile_pool.lifetime, buffer.num * sizeof(float));
    memcpy(new_pool.position, projectile_pool.position, buffer.num * sizeof(glm::vec3));
    memcpy(new_pool.velocity, projectile_pool.velocity, buffer.num * sizeof(glm::vec3));

    free(buffer.buffer);
    buffer = new_buffer;

    projectile_pool = new_pool;
}

void projectile_manager::destroy_instance(unsigned index) {
    auto last_id = buffer.num - 1;

    projectile_pool.mass[index] = projectile_pool.mass[last_id];
    projectile_pool.lifetime[index] = projectile_pool.lifetime[last_id];
    projectile_pool.position[index] = projectile_pool.position[last_id];
    projectile_pool.velocity[index] = projectile_pool.velocity[last_id];

    --buffer.num;
}

void
projectile_manager::spawn(glm::vec3 pos, glm::vec3 dir) {
    if (buffer.num >= buffer.allocated) {
        assert(buffer.allocated == 0);
        printf("Resizing projectiles buffer. Please adjust initial.\n");
        create_projectile_data(std::max(1u, buffer.allocated) * 2);
        return;
    }

    auto index = buffer.num++;

    projectile_pool.position[index] = pos;
    projectile_pool.velocity[index] = dir * initial_speed;
    projectile_pool.lifetime[index] = initial_lifetime;
}

void projectile_linear_manager::simulate(float dt) {
    for (auto i = 0u; i < buffer.num; ) {
        auto new_pos = projectile_pool.position[i] + projectile_pool.velocity[i] * dt;

        auto hit = phys_raycast_generic(projectile_pool.position[i], new_pos,
            phy->ghostObj, phy->dynamicsWorld);

        if (hit.hit) {
            new_pos = hit.hitCoord;
            projectile_pool.velocity[i] = glm::vec3(0);
            projectile_pool.lifetime[i] = after_collision_lifetime;
        }

        projectile_pool.position[i] = new_pos;

        projectile_pool.lifetime[i] -= dt;

        if (projectile_pool.lifetime[i] <= 0.f) {
            destroy_instance(i);
            --i;
        }

        ++i;
    }
}

void
draw_projectiles(projectile_manager & proj_man, frame_data *frame)
{
    for (auto i = 0u; i < proj_man.buffer.num; i += INSTANCE_BATCH_SIZE) {
        auto batch_size = std::min(INSTANCE_BATCH_SIZE, proj_man.buffer.num - i);
        auto projectile_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

        for (auto j = 0u; j < batch_size; j++) {
            projectile_matrices.ptr[j] = mat_position(proj_man.projectile_pool.position[i+j]);
        }

        projectile_matrices.bind(1, frame);
        draw_mesh_instanced(projectile_hw, batch_size);
    }
}
