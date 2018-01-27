#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <libconfig.h>
#include <memory>

#include "component_manager.h"
#include "../enums/enums.h"

struct gas_producer_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        unsigned *gas_type;
        float *flow_rate;
        float *max_pressure;
        bool *enabled;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;
    void destroy_instance(instance i) override;
    void entity(c_entity e) override;

    instance_data get_instance_data(c_entity e) {
        instance_data d{};
        auto inst = lookup(e);

        d.entity = instance_pool.entity + inst.index;
        d.gas_type = instance_pool.gas_type + inst.index;

        d.flow_rate = instance_pool.flow_rate + inst.index;

        d.max_pressure = instance_pool.max_pressure + inst.index;

        d.enabled = instance_pool.enabled + inst.index;

        return d;
    }
};

struct gas_producer_component_stub : component_stub {
    gas_producer_component_stub() = default;

    unsigned gas_type{};

    float flow_rate{};

    float max_pressure{};

    void
    assign_component_to_entity(c_entity entity) override;

    static std::unique_ptr<component_stub> from_config(config_setting_t const *config);

    std::vector<std::string>
    get_dependencies() override;
};
