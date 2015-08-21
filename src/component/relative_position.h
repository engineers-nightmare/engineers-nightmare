#pragma once

#pragma once

#include "component_manager.h"

// position relative to ship component
// position -- relative to ship. :)
// glm::vec3
// mat      -- matrix for above
// glm::mat4

struct relative_position_component_manager : component_manager {
    struct relative_position_instance_data {
        c_entity *entity;
        glm::vec3 *position;
        glm::mat4 *mat;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;

    void destroy_instance(instance i) override;

    void entity(const c_entity &e) override;

    glm::vec3 & position(const c_entity &e) {
        auto inst = lookup(e);

        return instance_pool.position[inst.index];
    }

    glm::mat4 & mat(const c_entity &e) {
        auto inst = lookup(e);

        return instance_pool.mat[inst.index];
    }
};