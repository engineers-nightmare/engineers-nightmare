#pragma once

#include <unordered_map>
#include <string.h>

static unsigned entities_id_ref = 1;
struct c_entity {
    unsigned id;

    bool operator==(c_entity const &other) const {
        return this->id == other.id;
    }

    static c_entity spawn() {
        c_entity e = { entities_id_ref++ };
        return e;
    }
};

namespace std {
    template<>
    struct hash<c_entity> {
        size_t operator()(c_entity const &e) const {
            return hash<unsigned>()(e.id);
        }
    };
}

struct component_manager {
    struct instance {
        unsigned index;
    };

    struct component_buffer {
        unsigned num;
        unsigned allocated;
        void *buffer;
    } buffer;

    std::unordered_map<c_entity, unsigned> entity_instance_map;

    virtual void create_component_instance_data(unsigned count) = 0;

    void assign_entity(c_entity const &e) {
        auto i = make_instance(buffer.num);
        entity_instance_map[e] = i.index;
        entity(e);
        ++buffer.num;
    }

    virtual void entity(c_entity const &e) = 0;

    bool exists(c_entity const & e) {
        return entity_instance_map.find(e) != entity_instance_map.end();
    }

    instance lookup(c_entity const &e) {
        return make_instance(entity_instance_map.find(e)->second);
    }

    instance make_instance(unsigned i) {
        return { i };
    }

    void destroy_entity_instance(c_entity const &e) {
        if (exists(e)) {
            auto i = lookup(e);
            destroy_instance(i);
        }
    }

    virtual void destroy_instance(instance i) = 0;

    virtual ~component_manager() {
        // allocated in derived create_component_instance_data() calls
        free(buffer.buffer);
        buffer.buffer = nullptr;
    }
};
