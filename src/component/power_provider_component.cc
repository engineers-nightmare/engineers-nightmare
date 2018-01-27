/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "power_provider_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
power_provider_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(unsigned) * count + align_size<unsigned>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.max_provided = align_ptr((float *)(new_pool.entity + count));
    new_pool.provided = align_ptr((float *)(new_pool.max_provided + count));
    new_pool.network = align_ptr((unsigned *)(new_pool.provided + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.max_provided, instance_pool.max_provided, buffer.num * sizeof(float));
    memcpy(new_pool.provided, instance_pool.provided, buffer.num * sizeof(float));
    memcpy(new_pool.network, instance_pool.network, buffer.num * sizeof(unsigned));

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
    instance_pool.network[i.index] = instance_pool.network[last_index];

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

void
power_provider_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.power_provider_component_man;

    man.assign_entity(entity);

    auto data = man.get_instance_data(entity);        

    *data.max_provided = 0;

    *data.provided = 0;

    *data.network = 0;

    *data.max_provided = max_provided;
};

std::unique_ptr<component_stub> power_provider_component_stub::from_config(const config_setting_t *config) {
    auto power_provider_stub = std::make_unique<power_provider_component_stub>();

    auto max_provided_member = config_setting_get_member(config, "max_provided");
    power_provider_stub->max_provided = config_setting_get_float(max_provided_member);

    return std::move(power_provider_stub);
}

std::vector<std::string> power_provider_component_stub::get_dependencies() {
    return {
        
    };
}
