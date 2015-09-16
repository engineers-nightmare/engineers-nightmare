#pragma once

#include "component_manager.h"

// power component
// required_power -- power required for this entity
// unsigned
// powered -- connected to power
// bool

struct power_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        unsigned *required_power;
        bool *powered;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    unsigned & required_power(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.required_power[inst.index];
    }

    bool & powered(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.powered[inst.index];
    }
};
