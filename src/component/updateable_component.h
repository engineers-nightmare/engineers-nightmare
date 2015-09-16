#pragma once

#include "component_manager.h"

// updateable component
// updated -- something on this entity changed this frame
// bool

struct updateable_component_manager : component_manager {
    struct switchable_instance_data {
        c_entity *entity;
        bool *updated;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    bool & updated(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.updated[inst.index];
    }
};