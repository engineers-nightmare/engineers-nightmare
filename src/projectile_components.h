#pragma once

#include <glm/glm.hpp>
#include "component/component.h"

struct projectile_manager : component_manager {
    struct projectile_instance_data {
        unsigned num;
        unsigned allocated;
        void *buffer;

        c_entity *entity;
        float *mass;
        float *lifetime;
        glm::vec3 *position;
        glm::vec3 *velocity;
    } projectile_pool;

    static constexpr float initial_lifetime = 10.f;
    static constexpr float after_collision_lifetime = 0.5f;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    virtual void simulate(float dt) = 0;

    void spawn(glm::vec3 pos, glm::vec3 vel);

    void draw();

    float mass(instance i) {
        return projectile_pool.mass[i.index];
    }

    float lifetime(instance i) {
        return projectile_pool.lifetime[i.index];
    }

    glm::vec3 position(instance i) {
        return projectile_pool.position[i.index];
    }

    glm::vec3 velocity(instance i) {
        return projectile_pool.velocity[i.index];
    }

    void set_mass(instance i, float mass) {
        projectile_pool.mass[i.index] = mass;
    }

    void set_lifetime(instance i, float lifetime) {
        projectile_pool.lifetime[i.index] = lifetime;
    }

    void set_position(instance i, glm::vec3 pos) {
        projectile_pool.position[i.index] = pos;
    }

    void set_velocity(instance i, glm::vec3 velocity) {
        projectile_pool.velocity[i.index] = velocity;
    }

    ~projectile_manager() {
        free(projectile_pool.buffer);
    }
};

struct projectile_linear_manager : projectile_manager {
    void simulate(float dt) override;
};

struct projectile_sine_manager : projectile_manager {
    void simulate(float dt) override;
};