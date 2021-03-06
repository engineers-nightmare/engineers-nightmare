/* THIS FILE IS AUTOGENERATED BY gen/gen_comps.py; DO NOT HAND-MODIFY */

#include <algorithm>
#include <string.h>
#include <memory>

#include "../memory.h"
#include "rotator_component.h"
#include "component_system_manager.h"

extern component_system_manager component_system_man;

void
rotator_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer{};
    instance_data new_pool{};

    size_t size = sizeof(c_entity) * count;
    size = sizeof(wire_filter_ptr) * count + align_size<wire_filter_ptr>(size);
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);
    size = sizeof(int) * count + align_size<int>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size += 16;   // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.filter = align_ptr((wire_filter_ptr *)(new_pool.entity + count));
    new_pool.rot_axis = align_ptr((glm::vec3 *)(new_pool.filter + count));
    new_pool.rot_offset = align_ptr((glm::vec3 *)(new_pool.rot_axis + count));
    new_pool.rot_dir = align_ptr((int *)(new_pool.rot_offset + count));
    new_pool.rot_speed = align_ptr((float *)(new_pool.rot_dir + count));
    new_pool.rot_cur_speed = align_ptr((float *)(new_pool.rot_speed + count));
    new_pool.rot_angle = align_ptr((float *)(new_pool.rot_cur_speed + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.filter, instance_pool.filter, buffer.num * sizeof(wire_filter_ptr));
    memcpy(new_pool.rot_axis, instance_pool.rot_axis, buffer.num * sizeof(glm::vec3));
    memcpy(new_pool.rot_offset, instance_pool.rot_offset, buffer.num * sizeof(glm::vec3));
    memcpy(new_pool.rot_dir, instance_pool.rot_dir, buffer.num * sizeof(int));
    memcpy(new_pool.rot_speed, instance_pool.rot_speed, buffer.num * sizeof(float));
    memcpy(new_pool.rot_cur_speed, instance_pool.rot_cur_speed, buffer.num * sizeof(float));
    memcpy(new_pool.rot_angle, instance_pool.rot_angle, buffer.num * sizeof(float));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
rotator_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.filter[i.index] = instance_pool.filter[last_index];
    instance_pool.rot_axis[i.index] = instance_pool.rot_axis[last_index];
    instance_pool.rot_offset[i.index] = instance_pool.rot_offset[last_index];
    instance_pool.rot_dir[i.index] = instance_pool.rot_dir[last_index];
    instance_pool.rot_speed[i.index] = instance_pool.rot_speed[last_index];
    instance_pool.rot_cur_speed[i.index] = instance_pool.rot_cur_speed[last_index];
    instance_pool.rot_angle[i.index] = instance_pool.rot_angle[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
rotator_component_manager::entity(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of rotator buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}

void
rotator_component_stub::assign_component_to_entity(c_entity entity) {
    auto &man = component_system_man.managers.rotator_component_man;

    man.assign_entity(entity);

    auto data = man.get_instance_data(entity);

    *data.filter = {};
    *data.rot_axis = rot_axis;
    *data.rot_offset = rot_offset;
    *data.rot_dir = rot_dir;
    *data.rot_speed = rot_speed;
    *data.rot_cur_speed = 0.0;
    *data.rot_angle = 0.0;
};

std::unique_ptr<component_stub> rotator_component_stub::from_config(const config_setting_t *config) {
    auto rotator_stub = std::make_unique<rotator_component_stub>();

    rotator_stub->rot_axis = load_value_from_config<glm::vec3>(config, "rot_axis");
    rotator_stub->rot_offset = load_value_from_config<glm::vec3>(config, "rot_offset");
    rotator_stub->rot_dir = load_value_from_config<int>(config, "rot_dir");
    rotator_stub->rot_speed = load_value_from_config<float>(config, "rot_speed");

    return std::move(rotator_stub);
}

std::vector<std::string> rotator_component_stub::get_dependencies() {
    return {
        "position",
    };
}
