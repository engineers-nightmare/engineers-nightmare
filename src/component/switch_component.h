#pragma once

#include "component_manager.h"

// switch component
// no data

struct switch_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;
};