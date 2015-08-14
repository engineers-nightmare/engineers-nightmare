#pragma once

#include <unordered_map>
#include <glm/glm.hpp>

//extern hw_mesh *projectile_hw;

struct c_entity {
    unsigned id;
};

struct projectile_manager {
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

    projectile_manager(unsigned count);

    void create_projectile_instance_data(unsigned size);

    std::unordered_map<unsigned, unsigned> map;

    struct instance {
        unsigned i;
    };

    instance make_instance(unsigned i) {
        instance inst = { i };
        return inst;
    }

    instance lookup(c_entity e) {
        return make_instance(map.find(e.id)->second);
    }

    float mass(instance i) {
        return projectile_pool.mass[i.i];
    }

    float lifetime(instance i) {
        return projectile_pool.lifetime[i.i];
    }

    glm::vec3 position(instance i) {
        return projectile_pool.position[i.i];
    }

    glm::vec3 velocity(instance i) {
        return projectile_pool.velocity[i.i];
    }

    void set_mass(instance i, float mass) {
        projectile_pool.mass[i.i] = mass;
    }

    void set_lifetime(instance i, float lifetime) {
        projectile_pool.lifetime[i.i] = lifetime;
    }

    void set_position(instance i, glm::vec3 pos) {
        projectile_pool.position[i.i] = pos;
    }

    void set_velocity(instance i, glm::vec3 velocity) {
        projectile_pool.velocity[i.i] = velocity;
    }

    void spawn(glm::vec3 pos, glm::vec3 vel);

    void draw();

    void simulate(float dt);

    void destroy(instance i);
    
    ~projectile_manager() {
        free(projectile_pool.mass);
        free(projectile_pool.lifetime);
        free(projectile_pool.position);
        free(projectile_pool.velocity);
    }
};
