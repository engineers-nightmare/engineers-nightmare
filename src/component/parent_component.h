#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <libconfig.h>
#include <memory>

#include "component_manager.h"
#include "../enums/enums.h"

struct parent_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        c_entity *parent;
        glm::mat4 *local_mat;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;
    void destroy_instance(instance i) override;
    void entity(c_entity e) override;

    instance_data get_instance_data(c_entity e) {
        instance_data d{};
        auto inst = lookup(e);

        d.entity = instance_pool.entity + inst.index;
        d.parent = instance_pool.parent + inst.index;

        d.local_mat = instance_pool.local_mat + inst.index;

        return d;
    }
};

struct parent_component_stub : component_stub {
    parent_component_stub() = default;

    glm::mat4 local_mat{};

    void
    assign_component_to_entity(c_entity entity) override;

    static std::unique_ptr<component_stub> from_config(config_setting_t const *config);

    std::vector<std::string>
    get_dependencies() override;
};
