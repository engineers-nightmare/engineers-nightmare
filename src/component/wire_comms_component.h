#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_comps.py; DO NOT HAND-MODIFY */

#include <libconfig.h>
#include <memory>

#include "component_manager.h"
#include "../enums/enums.h"
#include "wire_filter.h"

struct wire_comms_component_manager : component_manager<wire_comms_component_manager> {
    struct instance_data {
        c_entity *entity;
        unsigned *network;
        char const * *label;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;
    void destroy_instance(instance i) override;
    void entity(c_entity e) override;

    static const char* get_ui_name() {
        return "Wire Comms";
    }

    instance_data get_instance_data(c_entity e) {
        instance_data d{};
        auto inst = lookup(e);

        d.entity = instance_pool.entity + inst.index;
        d.network = instance_pool.network + inst.index;
        d.label = instance_pool.label + inst.index;

        return d;
    }
};

struct wire_comms_component_stub : component_stub {
    wire_comms_component_stub() = default;


    void
    assign_component_to_entity(c_entity entity) override;

    static std::unique_ptr<component_stub> from_config(config_setting_t const *config);

    std::vector<std::string>
    get_dependencies() override;
};
