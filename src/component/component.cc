#include <stdlib.h>
#include <string.h>

#include "component.h"
#include <algorithm>

void
power_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    power_instance_data new_pool;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(bool) * count + align_size<bool>(size);
    size = sizeof(bool) * count + align_size<bool>(size);

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = (c_entity *)new_buffer.buffer;

    new_pool.powered = align_ptr((bool *)(new_pool.entity + count));
    new_pool.enabled = align_ptr((bool *)(new_pool.powered + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.powered, instance_pool.powered, buffer.num * sizeof(bool));
    memcpy(new_pool.enabled, instance_pool.enabled, buffer.num * sizeof(bool));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
power_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.powered[i.index] = instance_pool.powered[last_index];
    instance_pool.enabled[i.index] = instance_pool.enabled[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

power_component_manager::power_instance_data
power_component_manager::get_next_component(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of power_component buffer. Please adjust");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto i = assign_entity(e, buffer.num++);

    power_instance_data comp;
    comp.entity = &instance_pool.entity[i.index];
    comp.powered = &instance_pool.powered[i.index];
    comp.enabled = &instance_pool.enabled[i.index];

    *comp.entity = e;

    return comp;
}

void
gas_production_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    gas_production_instance_data new_pool;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(unsigned) * count + align_size<unsigned>(size);
    size = sizeof(float) * count + align_size<float>(size);

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = (c_entity *)new_buffer.buffer;

    new_pool.gas_type = align_ptr((unsigned *)(new_pool.entity + count));
    new_pool.flow_rate = align_ptr((float *)(new_pool.gas_type + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.gas_type, instance_pool.gas_type, buffer.num * sizeof(unsigned));
    memcpy(new_pool.flow_rate, instance_pool.flow_rate, buffer.num * sizeof(float));

    free(buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
gas_production_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.gas_type[i.index] = instance_pool.gas_type[last_index];
    instance_pool.flow_rate[i.index] = instance_pool.flow_rate[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

gas_production_component_manager::gas_production_instance_data
gas_production_component_manager::get_next_component(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of power_component buffer. Please adjust");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto i = assign_entity(e, buffer.num++);

    gas_production_instance_data comp;
    comp.entity = &instance_pool.entity[i.index];
    comp.gas_type = &instance_pool.gas_type[i.index];
    comp.flow_rate = &instance_pool.flow_rate[i.index];

    *comp.entity = e;

    return comp;
}

void
relative_position_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    relative_position_instance_data new_pool;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = (c_entity *)new_buffer.buffer;

    new_pool.position = align_ptr((glm::vec3 *)(new_pool.entity + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.position, instance_pool.position, buffer.num * sizeof(glm::vec3));

    free(new_buffer.buffer);
    buffer = new_buffer;

    instance_pool = new_pool;
}

void
relative_position_component_manager::destroy_instance(instance i) {
    auto last_index = buffer.num - 1;
    auto last_entity = instance_pool.entity[last_index];
    auto current_entity = instance_pool.entity[i.index];

    instance_pool.entity[i.index] = instance_pool.entity[last_index];
    instance_pool.position[i.index] = instance_pool.position[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

relative_position_component_manager::relative_position_instance_data
relative_position_component_manager::get_next_component(c_entity e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of relative_position buffer. Please adjust");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto i = assign_entity(e, buffer.num++);

    relative_position_instance_data comp;
    comp.entity = &instance_pool.entity[i.index];
    comp.position = &instance_pool.position[i.index];

    *comp.entity = e;

    return comp;
}