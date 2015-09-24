#pragma once

#include "component_manager.h"

// switch component
// compare_result -- 1 if both inputs are within compare_epsilon
// float
// compare_epsilon
// float

struct sensor_comparator_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        float *compare_result;
        float *compare_epsilon;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    float & compare_result(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.compare_result[inst.index];
    }

    float & compare_epsilon(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.compare_epsilon[inst.index];
    }
};