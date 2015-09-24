#pragma once

#include <glm/glm.hpp>
#include "../mesh.h"
#include "../render_data.h"

struct projectile_manager {
    struct projectile_instance_data {
        float *lifetime;
        glm::vec3 *position;
        glm::vec3 *velocity;
    } projectile_pool;

    struct component_buffer {
        unsigned num;
        unsigned allocated;
        void *buffer;
    } buffer;

    float initial_speed = 10.f;
    float initial_lifetime = 10.f;
    float after_collision_lifetime = 1.f;

    projectile_manager() {
    }

    virtual void create_projectile_data(unsigned count);

    virtual void destroy_instance(unsigned index);

    virtual void simulate(float dt) = 0;

    virtual void spawn(glm::vec3 pos, glm::vec3 vel);

    virtual ~projectile_manager() {
        free(buffer.buffer);
        buffer.buffer = nullptr;
    }
};

struct projectile_linear_manager : projectile_manager {
    void simulate(float dt) override;
};


void
draw_projectiles(projectile_manager & proj_man, frame_data *frame);
