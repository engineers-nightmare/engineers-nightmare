#pragma once

#include <unordered_map>
#include <glm/glm.hpp>

struct c_entity {
    unsigned id;

    bool operator==(const c_entity &other) const {
        return this->id == other.id;
    }
};

namespace std {
    template<>
    struct std::hash<c_entity> {
        size_t operator()(c_entity const &e) const {
            return std::hash<unsigned>()(e.id);
        }
    };
}

struct component_manager {
    struct component_instance_data {
        unsigned num;
        unsigned allocated;
        void *buffer;
    } instance_pool;

    struct instance {
        unsigned index;
    };

    std::unordered_map<c_entity, unsigned> entity_instance_map;
    
    virtual void create_component_instance_data(unsigned count) = 0;

    instance lookup(c_entity e) {
        return make_instance(entity_instance_map.find(e)->second);
    }

    instance make_instance(unsigned i) {
        return { i };
    }

    void destroy_instance(c_entity e) {
        auto i = lookup(e);
        destroy_instance(i);
    }

    virtual void destroy_instance(instance i) = 0;

    virtual ~component_manager() {
        free(instance_pool.buffer);
    }
};

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
};

struct projectile_linear_manager : projectile_manager {
    void simulate(float dt) override;
};

struct projectile_sine_manager : projectile_manager {
    void simulate(float dt) override;
};
