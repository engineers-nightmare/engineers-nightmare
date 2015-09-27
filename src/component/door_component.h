#pragma once

#include "component_manager.h"
#include "../mesh.h"

// door component

struct door_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        hw_mesh **mesh;
        float *pos;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    hw_mesh * & mesh(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.mesh[inst.index];
    }

    float & pos(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.pos[inst.index];
    }
};

