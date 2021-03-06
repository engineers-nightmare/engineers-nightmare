/* THIS FILE IS AUTOGENERATED BY gen/gen_comps.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "parent_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
parent_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size = sizeof(c_entity) * count + align_size<c_entity>(size);
    size = sizeof(glm::mat4) * count + align_size<glm::mat4>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.parent = align_ptr((c_entity *)(new_pool.entity + count));
    new_pool.local_mat = align_ptr((glm::mat4 *)(new_pool.parent + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.parent, instance_pool.parent, buffer.num * sizeof(c_entity));
    memcpy(new_pool.local_mat, instance_pool.local_mat, buffer.num * sizeof(glm::mat4));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
parent_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.parent[i.index] = instance_pool.parent[last_index];
    instance_pool.local_mat[i.index] = instance_pool.local_mat[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
parent_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of parent buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}

void
parent_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.parent_component_man;

    man.assign_entity(entity);

    auto data = man.get_instance_data(entity);

    *data.parent = c_entity{};
    *data.local_mat = local_mat;
};

std::unique_ptr<component_stub> parent_component_stub::from_config(const config_setting_t *config) {
    auto parent_stub = std::make_unique<parent_component_stub>();

    parent_stub->local_mat = load_value_from_config<glm::mat4>(config, "local_mat");

    return std::move(parent_stub);
}

std::vector<std::string> parent_component_stub::get_dependencies() {
    return {
        "position",
    };
}
