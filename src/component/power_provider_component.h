#pragma once

#include "component_manager.h"

// power component
// max_provided -- max power able to be provided
// unsigned
// provided -- current power provided
// unsigned

struct power_provider_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        unsigned *max_provided;
        unsigned *provided;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    unsigned & max_provided(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.max_provided[inst.index];
    }

    unsigned & provided(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.provided[inst.index];
    }
};
