#pragma once

#include <glm/glm.hpp>
#include "mesh.h"
#include "render_data.h"

struct particle_manager {
    struct particle_instance_data {
        float *lifetime;
        glm::vec3 *position;
        glm::vec3 *velocity;
    } particle_pool;

    struct component_buffer {
        unsigned num;
        unsigned allocated;
        void *buffer;
    } buffer;

    float initial_speed = 10.f;
    float initial_lifetime = 10.f;
    float after_collision_lifetime = 1.f;
    GLuint vao;

    particle_manager();

    virtual void create_particle_data(unsigned count);

    virtual void destroy_instance(unsigned index);

    void simulate(float dt);

    virtual void spawn(glm::vec3 pos, glm::vec3 vel, float lifetime);

    virtual ~particle_manager() {
        free(buffer.buffer);
        buffer.buffer = nullptr;
    }
};


void
draw_particles(particle_manager *man, frame_data *frame);
