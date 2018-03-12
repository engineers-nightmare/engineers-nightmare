/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "placeable_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
placeable_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
placeable_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
placeable_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of placeable buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}

void
placeable_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.placeable_component_man;

    man.assign_entity(entity);
};

std::unique_ptr<component_stub> placeable_component_stub::from_config(const config_setting_t *config) {
    auto placeable_stub = std::make_unique<placeable_component_stub>();

    placeable_stub->rot = load_value_from_config<int>(config, "rot");

    placeable_stub->place = load_value_from_config<placement>(config, "place");

    return std::move(placeable_stub);
}

std::vector<std::string> placeable_component_stub::get_dependencies() {
    return {
        
    };
}
