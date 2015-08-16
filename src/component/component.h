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
