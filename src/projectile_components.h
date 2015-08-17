#pragma once

#include <glm/glm.hpp>
#include "component/component.h"
#include <algorithm>

struct projectile_manager : component_manager {
    struct projectile_instance_data {
        c_entity *entity;
        float *mass;
        float *lifetime;
        glm::vec3 *position;
        glm::vec3 *velocity;
    } projectile_pool;

    static constexpr float initial_lifetime = 10.f;
    static constexpr float after_collision_lifetime = 0.5f;

    c_entity owner;

    projectile_manager(c_entity owner) : owner(owner) {
    }

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    virtual void simulate(float dt) = 0;

    void spawn(glm::vec3 pos, glm::vec3 vel);

    void draw();

    float mass(c_entity e) {
        auto inst = lookup(owner);

        return projectile_pool.mass[inst.index];
    }

    void mass(c_entity e, float mass) {
        auto inst = lookup(owner);

        projectile_pool.mass[inst.index] = mass;
    }

    float lifetime(c_entity e) {
        auto inst = lookup(owner);

        return projectile_pool.lifetime[inst.index];
    }

    void lifetime(c_entity e, float lifetime) {
        auto inst = lookup(owner);

        projectile_pool.lifetime[inst.index] = lifetime;
    }

    glm::vec3 position(c_entity e) {
        auto inst = lookup(owner);

        return projectile_pool.position[inst.index];
    }

    void position(c_entity e, glm::vec3 pos) {
        auto inst = lookup(owner);

        projectile_pool.position[inst.index] = pos;
    }

    glm::vec3 velocity(c_entity e) {
        auto inst = lookup(owner);

        return projectile_pool.velocity[inst.index];
    }

    void velocity(c_entity e, glm::vec3 velocity) {
        auto inst = lookup(owner);

        projectile_pool.velocity[inst.index] = velocity;
    }
};

struct projectile_linear_manager : projectile_manager {
    explicit projectile_linear_manager(const c_entity& owner)
        : projectile_manager(owner) {
    }

    void entity(const c_entity &e) override {
        if (buffer.num >= buffer.allocated) {
            printf("Increasing size of projectile_linear buffer. Please adjust");
            create_component_instance_data(std::max(1u, buffer.allocated) * 2);
        }

        auto inst = lookup(e);

        projectile_pool.entity[inst.index] = e;
    }

    void simulate(float dt) override;

    ~projectile_linear_manager() {
        // allocated in derived create_component_instance_data() calls
        free(buffer.buffer);
        buffer.buffer = nullptr;
    }
};

struct projectile_sine_manager : projectile_manager {
    explicit projectile_sine_manager(const c_entity& owner)
        : projectile_manager(owner) {
    }

    void entity(const c_entity &e) override {
        auto inst = lookup(e);

        projectile_pool.entity[inst.index] = e;
    }

    void simulate(float dt) override;

    ~projectile_sine_manager() {
        // allocated in derived create_component_instance_data() calls
        free(buffer.buffer);
        buffer.buffer = nullptr;
    }
};