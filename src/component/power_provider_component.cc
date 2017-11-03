#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "power_provider_component.h"

/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

std::shared_ptr<component_stub>
power_provider_stub_from_config(const config_setting_t *power_provider_config) {
    auto power_provider_stub = std::make_shared<power_provider_component_stub>();

    auto max_provided_member = config_setting_get_member(power_provider_config, "max_provided");
    power_provider_stub->max_provided = config_setting_get_float(max_provided_member);

    return power_provider_stub;
};

extern std::unordered_map<std::string, std::function<std::shared_ptr<component_stub>(config_setting_t *)>> component_stub_generators;

void
power_provider_component_stub::register_generator() {
    component_stub_generators["power_provider"] = power_provider_stub_from_config;
}

void
power_provider_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    instance_data new_pool;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.max_provided = align_ptr((float *)(new_pool.entity + count));
    new_pool.provided = align_ptr((float *)(new_pool.max_provided + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.max_provided, instance_pool.max_provided, buffer.num * sizeof(float));
    memcpy(new_pool.provided, instance_pool.provided, buffer.num * sizeof(float));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
power_provider_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.max_provided[i.index] = instance_pool.max_provided[last_index];
    instance_pool.provided[i.index] = instance_pool.provided[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
power_provider_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of power_provider buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}
