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
    struct hash<c_entity> {
        size_t operator()(c_entity const &e) const {
            return std::hash<unsigned>()(e.id);
        }
    };
}

struct component_manager {
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

    virtual ~component_manager() {}
};

// power component
// has-power // connected to power
// bool
// enabled   // switched on
// bool

struct power_component_manager : component_manager {
    struct power_instance_data {
        unsigned num;
        unsigned allocated;
        void *buffer;

        c_entity *entity;
        bool *powered;
        bool *enabled;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;
};

// gas production component
// type // not set yet
// unsigned
// rate // rate of flow
// float

struct gas_production_component_manager : component_manager {
    struct gas_production_instance_data {
        unsigned num;
        unsigned allocated;
        void *buffer;

        c_entity *entity;
        unsigned *gas_type;
        float *rate;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;
};

// position relative to ship component
// position // relative to ship. :)
// glm::vec3

struct relative_position_component_manager : component_manager {
    struct relative_position_instance_data {
        unsigned num;
        unsigned allocated;
        void *buffer;

        c_entity *entity;
        glm::vec3 *position;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;
};
