#pragma once

#include <glm/glm.hpp>
#include "../mesh.h"

struct projectile_manager {
    struct projectile_instance_data {
        float *mass;
        float *lifetime;
        glm::vec3 *position;
        glm::vec3 *velocity;
        hw_mesh *mesh;
    } projectile_pool;

    struct component_buffer {
        unsigned num;
        unsigned allocated;
        void *buffer;
    } buffer;

    float initial_speed = 10.f;
    float initial_lifetime = 10.f;
    float after_collision_lifetime = 0.5f;

    projectile_manager(){
    }

    virtual void create_projectile_data(unsigned count);

    virtual void destroy_instance(unsigned index);

    virtual void simulate(float dt) = 0;

    virtual void spawn(glm::vec3 pos, glm::vec3 vel, hw_mesh m);

    virtual void draw();

    float & mass(unsigned index) {
        return projectile_pool.mass[index];
    }

    float & lifetime(unsigned index) {
        return projectile_pool.lifetime[index];
    }

    glm::vec3 & position(unsigned index) {
        return projectile_pool.position[index];
    }

    glm::vec3 & velocity(unsigned index) {
        return projectile_pool.velocity[index];
    }

    hw_mesh & mesh(unsigned index) {
        return projectile_pool.mesh[index];
    }

    virtual ~projectile_manager() {
        free(buffer.buffer);
        buffer.buffer = nullptr;
    }
};

struct projectile_linear_manager : projectile_manager {
    void simulate(float dt) override;
};

struct projectile_sine_manager : projectile_manager {
    void simulate(float dt) override;
};