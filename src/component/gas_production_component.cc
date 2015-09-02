#include <algorithm>
#include "../memory.h"
#include "gas_production_component.h"

void
gas_production_component_manager::create_component_instance_data(unsigned count) {
    if (count <= buffer.allocated)
        return;

    component_buffer new_buffer;
    gas_production_instance_data new_pool;

    size_t size = sizeof(c_entity) * count;
    size = sizeof(unsigned) * count + align_size<unsigned>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size = sizeof(float) * count + align_size<float>(size);
    size += alignof(float); // for worst-case misalignment of initial ptr

    new_buffer.buffer = malloc(size);
    new_buffer.num = buffer.num;
    new_buffer.allocated = count;
    memset(new_buffer.buffer, 0, size);

    new_pool.entity = align_ptr((c_entity *)new_buffer.buffer);
    new_pool.gas_type = align_ptr((unsigned *)(new_pool.entity + count));
    new_pool.flow_rate = align_ptr((float *)(new_pool.gas_type + count));
    new_pool.max_pressure = align_ptr((float *)(new_pool.flow_rate + count));

    memcpy(new_pool.entity, instance_pool.entity, buffer.num * sizeof(c_entity));
    memcpy(new_pool.gas_type, instance_pool.gas_type, buffer.num * sizeof(unsigned));
    memcpy(new_pool.flow_rate, instance_pool.flow_rate, buffer.num * sizeof(float));
    memcpy(new_pool.max_pressure, instance_pool.max_pressure, buffer.num * sizeof(float));

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
    instance_pool.max_pressure[i.index] = instance_pool.max_pressure[last_index];

    entity_instance_map[last_entity] = i.index;
    entity_instance_map.erase(current_entity);

    --buffer.num;
}

void
gas_production_component_manager::entity(c_entity const &e) {
    if (buffer.num >= buffer.allocated) {
        printf("Increasing size of gas_production buffer. Please adjust\n");
        create_component_instance_data(std::max(1u, buffer.allocated) * 2);
    }

    auto inst = lookup(e);

    instance_pool.entity[inst.index] = e;
}
