#include <stdlib.h>
#include <string.h>

#include "component.h"

void power_component_manager::create_component_instance_data(unsigned count) {
    if (count <= instance_pool.allocated)
        return;

    power_instance_data data;

    auto entity_size = sizeof(c_entity) * count;

    auto powered_size = sizeof(bool) * count;
    powered_size += powered_size % alignof(bool);

    auto enabled_size = sizeof(bool) * count;
    enabled_size += enabled_size % alignof(bool);

    auto b2 = count * (sizeof(c_entity) + sizeof(bool) * 2);
    const auto bytes = entity_size + powered_size + enabled_size;

    assert(b2 == bytes);

    data.buffer = malloc(bytes);
    data.num = instance_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.powered = (bool *)(data.entity + count);
    data.powered = (bool*)((size_t)data.powered + alignof(bool)-1 & ~(alignof(bool)-1));

    data.enabled = (bool *)(data.powered + count);
    data.enabled = (bool*)((size_t)data.enabled + alignof(bool)-1 & ~(alignof(bool)-1));

    memcpy(data.entity, instance_pool.entity, instance_pool.num * sizeof(c_entity));
    memcpy(data.powered, instance_pool.powered, instance_pool.num * sizeof(bool));
    memcpy(data.enabled, instance_pool.enabled, instance_pool.num * sizeof(bool));

    free(instance_pool.buffer);
    instance_pool = data;
}

void power_component_manager::destroy_instance(instance i) {
    auto last_id = instance_pool.num - 1;
    auto last = instance_pool.entity[last_id];

    instance_pool.entity[i.index] = instance_pool.entity[last_id];
    instance_pool.powered[i.index] = instance_pool.powered[last_id];
    instance_pool.enabled[i.index] = instance_pool.enabled[last_id];

    entity_instance_map[last] = i.index;
    entity_instance_map.erase(last);

    --instance_pool.num;
}

void gas_production_component_manager::create_component_instance_data(unsigned count) {
    if (count <= instance_pool.allocated)
        return;

    gas_production_instance_data data;

    auto entity_size = sizeof(c_entity) * count;

    auto gas_type_size = sizeof(unsigned) * count;
    gas_type_size += gas_type_size % alignof(unsigned);

    auto rate_size = sizeof(float) * count;
    rate_size += rate_size % alignof(float);

    auto b2 = count * (sizeof(c_entity) + sizeof(unsigned) + sizeof(float));
    const auto bytes = entity_size + gas_type_size + rate_size;

    assert(b2 == bytes);

    data.buffer = malloc(bytes);
    data.num = instance_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.gas_type = (unsigned *)(data.entity + count);
    data.gas_type = (unsigned*)((size_t)data.gas_type + alignof(unsigned)-1 & ~(alignof(unsigned)-1));

    data.rate = (float *)(data.gas_type + count);
    data.rate = (float*)((size_t)data.rate + alignof(float)-1 & ~(alignof(float)-1));

    memcpy(data.entity, instance_pool.entity, instance_pool.num * sizeof(c_entity));
    memcpy(data.gas_type, instance_pool.gas_type, instance_pool.num * sizeof(unsigned));
    memcpy(data.rate, instance_pool.rate, instance_pool.num * sizeof(float));

    free(instance_pool.buffer);
    instance_pool = data;
}

void gas_production_component_manager::destroy_instance(instance i) {
    auto last_id = instance_pool.num - 1;
    auto last = instance_pool.entity[last_id];

    instance_pool.entity[i.index] = instance_pool.entity[last_id];
    instance_pool.gas_type[i.index] = instance_pool.gas_type[last_id];
    instance_pool.rate[i.index] = instance_pool.rate[last_id];

    entity_instance_map[last] = i.index;
    entity_instance_map.erase(last);

    --instance_pool.num;
}

void relative_position_component_manager::create_component_instance_data(unsigned count) {
    if (count <= instance_pool.allocated)
        return;

    relative_position_instance_data data;

    auto entity_size = sizeof(c_entity) * count;

    auto pos_size = sizeof(glm::vec3) * count;
    pos_size += pos_size % alignof(glm::vec3);

    auto b2 = count * (sizeof(c_entity) + sizeof(glm::vec3));
    const auto bytes = entity_size + pos_size;

    assert(b2 == bytes);

    data.buffer = malloc(bytes);
    data.num = instance_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.position = (glm::vec3 *)(data.entity + count);
    data.position = (glm::vec3 *)((size_t)data.position + alignof(glm::vec3)-1 & ~(alignof(glm::vec3)-1));

    memcpy(data.entity, instance_pool.entity, instance_pool.num * sizeof(c_entity));
    memcpy(data.position, instance_pool.position, instance_pool.num * sizeof(glm::vec3));
   
    free(instance_pool.buffer);
    instance_pool = data;
}

void relative_position_component_manager::destroy_instance(instance i) {
    auto last_id = instance_pool.num - 1;
    auto last = instance_pool.entity[last_id];

    instance_pool.entity[i.index] = instance_pool.entity[last_id];
    instance_pool.position[i.index] = instance_pool.position[last_id];

    entity_instance_map[last] = i.index;
    entity_instance_map.erase(last);

    --instance_pool.num;
}
