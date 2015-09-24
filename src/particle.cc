#include <algorithm>
#include <assert.h>

#include "common.h"
#include "particle.h"
#include "memory.h"
#include "mesh.h"
#include "physics.h"

hw_mesh *particle_hw;
sw_mesh *particle_sw;

void
particle_manager::create_particle_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    particle_instance_data new_pool;

    size_t size = sizeof(float) * count;
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);
    size += alignof(glm::vec3);     // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.lifetime = align_ptr((float *)new_buffer.buffer);
    new_pool.position = align_ptr((glm::vec3 *)(new_pool.lifetime + count));
    new_pool.velocity = align_ptr((glm::vec3 *)(new_pool.position + count));

    memcpy(new_pool.lifetime, particle_pool.lifetime, buffer.num * sizeof(float));
    memcpy(new_pool.position, particle_pool.position, buffer.num * sizeof(glm::vec3));
    memcpy(new_pool.velocity, particle_pool.velocity, buffer.num * sizeof(glm::vec3));

    free(buffer.buffer);
    buffer = new_buffer;

    particle_pool = new_pool;
}

void particle_manager::destroy_instance(unsigned index) {
    auto last_id = buffer.num - 1;

    particle_pool.lifetime[index] = particle_pool.lifetime[last_id];
    particle_pool.position[index] = particle_pool.position[last_id];
    particle_pool.velocity[index] = particle_pool.velocity[last_id];

    --buffer.num;
}

void
particle_manager::spawn(glm::vec3 pos, glm::vec3 dir, float lifetime) {
    if (buffer.num >= buffer.allocated) {
        assert(buffer.allocated == 0);
        printf("Resizing particles buffer. Please adjust initial.\n");
        create_particle_data(std::max(1u, buffer.allocated) * 2);
        return;
    }

    auto index = buffer.num++;

    particle_pool.position[index] = pos;
    particle_pool.velocity[index] = dir * initial_speed;
    particle_pool.lifetime[index] = lifetime;
}

void particle_manager::simulate(float dt) {
    for (auto i = 0u; i < buffer.num; ) {
        auto new_pos = particle_pool.position[i] + particle_pool.velocity[i] * dt;

        particle_pool.position[i] = new_pos;
        particle_pool.lifetime[i] -= dt;

        if (particle_pool.lifetime[i] <= 0.f) {
            destroy_instance(i);
        }
        else {
            ++i;
        }
    }
}

particle_manager::particle_manager() : vao(0)
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao); /* nothing to actually do here; we're pure gl_VertexID */
    glBindVertexArray(0);

    buffer.buffer = nullptr;
    buffer.num = 0;
    buffer.allocated = 0;
}


void
draw_particles(particle_manager *man, frame_data *frame)
{
    for (auto i = 0u; i < man->buffer.num; i += INSTANCE_BATCH_SIZE) {
        auto batch_size = std::min(INSTANCE_BATCH_SIZE, man->buffer.num - i);

        auto particle_params = frame->alloc_aligned<glm::vec4>(batch_size);

        for (auto j = 0u; j < batch_size; j++) {
            particle_params.ptr[j] = glm::vec4(man->particle_pool.position[i+j],
                                               man->particle_pool.lifetime[i+j]);
        }

        particle_params.bind(1, frame);

        glBindVertexArray(man->vao);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, 0, batch_size);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}
