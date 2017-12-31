/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "proximity_sensor_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
proximity_sensor_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(bool) * count + align_size<bool>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.range = align_ptr((float *)(new_pool.entity + count));
    new_pool.is_detected = align_ptr((bool *)(new_pool.range + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.range, instance_pool.range, buffer.num * sizeof(float));
    memcpy(new_pool.is_detected, instance_pool.is_detected, buffer.num * sizeof(bool));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
proximity_sensor_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.range[i.index] = instance_pool.range[last_index];
    instance_pool.is_detected[i.index] = instance_pool.is_detected[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
proximity_sensor_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of proximity_sensor buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}

void
proximity_sensor_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.proximity_sensor_component_man;

    man.assign_entity(entity);

    auto data = man.get_instance_data(entity);        

    *data.range = 0;

    *data.is_detected = false;

    *data.range = range;
};

std::unique_ptr<component_stub> proximity_sensor_component_stub::from_config(const config_setting_t *config) {
    auto proximity_sensor_stub = std::make_unique<proximity_sensor_component_stub>();

    auto range_member = config_setting_get_member(config, "range");
    proximity_sensor_stub->range = (float)config_setting_get_float(range_member);

    return std::move(proximity_sensor_stub);
}
