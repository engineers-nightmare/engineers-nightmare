#pragma once

#include "component_manager.h"

// switch component
// pressure -- atmo pressure. 0-1
// float

struct pressure_sensor_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        float *pressure;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    float & pressure(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.pressure[inst.index];
    }
};