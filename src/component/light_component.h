#pragma once

#include "component_manager.h"

// light component
// intensity -- 0-1
// glm::vec3

struct light_component_manager : component_manager {
    struct light_instance_data {
        c_entity *entity;
        float *intensity;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    float & intensity(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.intensity[inst.index];
    }
};
