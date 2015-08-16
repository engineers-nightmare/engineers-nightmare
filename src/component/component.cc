#include <stdlib.h>
#include <string.h>

#include "component.h"

void power_component_manager::create_component_instance_data(unsigned count) {
    if (count <= instance_pool.allocated)
        return;

    power_instance_data data;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(bool) * count + align_size<bool>(size);
    size = sizeof(bool) * count + align_size<bool>(size);

    data.buffer = malloc(size);
    data.num = instance_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.powered = align_ptr((bool *)(data.entity + count));
    data.enabled = align_ptr((bool *)(data.powered + count));

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

    size_t size = sizeof(c_entity) * count;
    size = sizeof(unsigned) * count + align_size<unsigned>(size);
    size = sizeof(float) * count + align_size<float>(size);

    data.buffer = malloc(size);
    data.num = instance_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.gas_type = align_ptr((unsigned *)(data.entity + count));
    data.flow_rate = align_ptr((float *)(data.gas_type + count));

    memcpy(data.entity, instance_pool.entity, instance_pool.num * sizeof(c_entity));
    memcpy(data.gas_type, instance_pool.gas_type, instance_pool.num * sizeof(unsigned));
    memcpy(data.flow_rate, instance_pool.flow_rate, instance_pool.num * sizeof(float));

    free(instance_pool.buffer);
    instance_pool = data;
}

void gas_production_component_manager::destroy_instance(instance i) {
    auto last_id = instance_pool.num - 1;
    auto last = instance_pool.entity[last_id];

    instance_pool.entity[i.index] = instance_pool.entity[last_id];
    instance_pool.gas_type[i.index] = instance_pool.gas_type[last_id];
    instance_pool.flow_rate[i.index] = instance_pool.flow_rate[last_id];

    entity_instance_map[last] = i.index;
    entity_instance_map.erase(last);

    --instance_pool.num;
}

void relative_position_component_manager::create_component_instance_data(unsigned count) {
    if (count <= instance_pool.allocated)
        return;

    relative_position_instance_data data;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(glm::vec3) * count + align_size<glm::vec3>(size);

    data.buffer = malloc(size);
    data.num = instance_pool.num;
    data.allocated = count;

    data.entity = (c_entity *)data.buffer;

    data.position = align_ptr((glm::vec3 *)(data.entity + count));

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
