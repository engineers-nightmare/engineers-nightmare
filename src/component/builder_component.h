#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <libconfig.h>
#include <memory>

#include "component_manager.h"
#include "../enums/enums.h"

struct builder_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        float *speed;
        float *rot_speed;
        glm::vec3 *desired_pos;
        int *build_index;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;
    void destroy_instance(instance i) override;
    void entity(c_entity e) override;

    instance_data get_instance_data(c_entity e) {
        instance_data d{};
        auto inst = lookup(e);

        d.entity = instance_pool.entity + inst.index;
        d.speed = instance_pool.speed + inst.index;

        d.rot_speed = instance_pool.rot_speed + inst.index;

        d.desired_pos = instance_pool.desired_pos + inst.index;

        d.build_index = instance_pool.build_index + inst.index;

        return d;
    }
};

struct builder_component_stub : component_stub {
    builder_component_stub() = default;

    float speed{};

    float rot_speed{};

    void
    assign_component_to_entity(c_entity entity) override;

    static std::unique_ptr<component_stub> from_config(config_setting_t const *config);
};
