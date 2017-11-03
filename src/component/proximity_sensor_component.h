#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <libconfig.h>
#include <memory>

#include "component_manager.h"

struct proximity_sensor_component_manager : component_manager {
    struct instance_data {
        c_entity *entity;
        float *range;
        bool *is_detected;
    } instance_pool;

    void create_component_instance_data(unsigned count) override;
    void destroy_instance(instance i) override;
    void entity(c_entity e) override;

    void register_stub_generator();

    instance_data get_instance_data(c_entity e) {
        instance_data d{};
        auto inst = lookup(e);

        d.entity = instance_pool.entity + inst.index;
        d.range = instance_pool.range + inst.index;
        d.is_detected = instance_pool.is_detected + inst.index;

        return d;
    }

    static proximity_sensor_component_manager* get_manager() {
        return dynamic_cast<proximity_sensor_component_manager*>(component_managers["proximity_sensor"].get());
    }
};

struct proximity_sensor_component_stub : component_stub {
    proximity_sensor_component_stub() : component_stub("proximity_sensor") {}

    float range{};

    void
    assign_component_to_entity(c_entity entity) {
        std::shared_ptr<component_manager> m = std::move(component_managers[name]);
        std::shared_ptr<proximity_sensor_component_manager> man = std::dynamic_pointer_cast<proximity_sensor_component_manager>(m);

        man->assign_entity(entity);
        auto data = man->get_instance_data(entity);        

        *data.range = 0;

        *data.is_detected = false;

        *data.range = range;
  };
};
