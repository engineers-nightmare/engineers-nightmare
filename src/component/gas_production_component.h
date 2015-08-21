#pragma once

#include "component_manager.h"

// gas production component
// gas_type     -- not set yet
// unsigned
// flow_rate    -- rate of flow
// float
// max_pressure -- don't fill past the line
// float

struct gas_production_component_manager : component_manager {
    struct gas_production_instance_data {
        c_entity *entity;
        unsigned *gas_type;
        float *flow_rate;
        float *max_pressure;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(const c_entity &e) override;

    unsigned & gas_type(const c_entity &e) {
        auto inst = lookup(e);

        return instance_pool.gas_type[inst.index];
    }

    float & flow_rate(const c_entity &e) {
        auto inst = lookup(e);

        return instance_pool.flow_rate[inst.index];
    }

    float & max_pressure(const c_entity &e) {
        auto inst = lookup(e);

        return instance_pool.max_pressure[inst.index];
    }
};
