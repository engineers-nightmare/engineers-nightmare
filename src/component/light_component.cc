/* THIS FILE IS AUTOGENERATED BY gen/gen_comps.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "light_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
light_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.intensity = align_ptr((float *)(new_pool.entity + count));
    new_pool.requested_intensity = align_ptr((float *)(new_pool.intensity + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.intensity, instance_pool.intensity, buffer.num * sizeof(float));
    memcpy(new_pool.requested_intensity, instance_pool.requested_intensity, buffer.num * sizeof(float));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
light_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.intensity[i.index] = instance_pool.intensity[last_index];
    instance_pool.requested_intensity[i.index] = instance_pool.requested_intensity[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
light_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of light buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}

void
light_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.light_component_man;

    man.assign_entity(entity);

    auto data = man.get_instance_data(entity);

    *data.intensity = intensity;
    *data.requested_intensity = 1;
};

std::unique_ptr<component_stub> light_component_stub::from_config(const config_setting_t *config) {
    auto light_stub = std::make_unique<light_component_stub>();

    light_stub->intensity = load_value_from_config<float>(config, "intensity");

    return std::move(light_stub);
}

std::vector<std::string> light_component_stub::get_dependencies() {
    return {
        "power",
        "wire_comms",
    };
}
