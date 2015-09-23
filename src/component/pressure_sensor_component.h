#pragma once

#include "component_manager.h"

// switch component
// pressure -- atmo pressure. 0-1
// float
// todo: fix below hack. used as we don't comms origin discriminate
// type -- 1-2
// char

struct pressure_sensor_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        float *pressure;
        unsigned *type;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    float & pressure(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.pressure[inst.index];
    }

    unsigned & type(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.type[inst.index];
    }
};