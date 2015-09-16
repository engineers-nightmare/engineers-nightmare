#pragma once

#include "component_manager.h"
#include <glm/glm.hpp>

// surface attachment component
// block -- attached to
// ivec3
// face  -- which face of block
// int

struct surface_attachment_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        glm::ivec3 *block;
        int *face;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(c_entity const &e) override;

    glm::ivec3 & block(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.block[inst.index];
    }

    int & face(c_entity const &e) {
        auto inst = lookup(e);

        return instance_pool.face[inst.index];
    }
};
