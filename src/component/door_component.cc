/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "door_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
door_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size = sizeof(const char*) * count + align_size<const char*>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(int) * count + align_size<int>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.mesh = align_ptr((const char* *)(new_pool.entity + count));
    new_pool.pos = align_ptr((float *)(new_pool.mesh + count));
    new_pool.desired_pos = align_ptr((float *)(new_pool.pos + count));
    new_pool.height = align_ptr((int *)(new_pool.desired_pos + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.mesh, instance_pool.mesh, buffer.num * sizeof(const char*));
    memcpy(new_pool.pos, instance_pool.pos, buffer.num * sizeof(float));
    memcpy(new_pool.desired_pos, instance_pool.desired_pos, buffer.num * sizeof(float));
    memcpy(new_pool.height, instance_pool.height, buffer.num * sizeof(int));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
door_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.mesh[i.index] = instance_pool.mesh[last_index];
    instance_pool.pos[i.index] = instance_pool.pos[last_index];
    instance_pool.desired_pos[i.index] = instance_pool.desired_pos[last_index];
    instance_pool.height[i.index] = instance_pool.height[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
door_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of door buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}

void
door_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.door_component_man;

    man.assign_entity(entity);

    auto data = man.get_instance_data(entity);        

    *data.mesh = mesh.c_str();

    *data.pos = 1;

    *data.desired_pos = 1;

    *data.height = 2;
};

std::unique_ptr<component_stub> door_component_stub::from_config(const config_setting_t *config) {
    auto door_stub = std::make_unique<door_component_stub>();

    door_stub->mesh = load_value_from_config<std::string>(config, "mesh");

    return std::move(door_stub);
}

std::vector<std::string> door_component_stub::get_dependencies() {
    return {
        "power", "reader", 
    };
}
