#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_comps.py; DO NOT HAND-MODIFY */

#include <libconfig.h>
#include <memory>

#include "component_manager.h"
#include "../enums/enums.h"
#include "wire_filter.h"

struct light_component_manager : component_manager<light_component_manager> {
    struct instance_data {
        c_entity *entity;
        wire_filter_ptr *filter;
        float *intensity;
        float *requested_intensity;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;
    void destroy_instance(instance i) override;
    void entity(c_entity e) override;

    static const char* get_ui_name() {
        return "Light";
    }

    instance_data get_instance_data(c_entity e) {
        instance_data d{};
        auto inst = lookup(e);

        d.entity = instance_pool.entity + inst.index;
        d.filter = instance_pool.filter + inst.index;
        d.intensity = instance_pool.intensity + inst.index;
        d.requested_intensity = instance_pool.requested_intensity + inst.index;

        return d;
    }
};

struct light_component_stub : component_stub {
    light_component_stub() = default;

    float intensity{};

    void
    assign_component_to_entity(c_entity entity) override;

    static std::unique_ptr<component_stub> from_config(config_setting_t const *config);

    std::vector<std::string>
    get_dependencies() override;
};
