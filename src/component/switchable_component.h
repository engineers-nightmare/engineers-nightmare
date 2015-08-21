#pragma once

#include "component_manager.h"

// switchable component
// enabled -- switched on
// bool

struct switchable_component_manager : component_manager {
    struct switchable_instance_data {
        c_entity *entity;
        bool *enabled;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(const c_entity &e) override;

    bool & enabled(const c_entity &e) {
        auto inst = lookup(e);

        return instance_pool.enabled[inst.index];
    }
};