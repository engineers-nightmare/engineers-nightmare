/* THIS FILE IS AUTOGENERATED BY gen/gen_comps.py; DO NOT HAND-MODIFY */

#include <vector>
#include "../component/component_system_manager.h"

extern component_system_manager component_system_man;

static void add_filter(std::vector<filter_ui_state> &filters, int field_id, wire_filter_ptr const & w, char const *name) {
    filters.emplace_back();
    auto & f = filters.back();
    f.component_name = name;
    f.field_id = field_id;
    f.type = w.type;

    if (w.wrapped) {
        strcpy(f.filter.data(), w.wrapped->c_str());
    }
    else {
        f.filter[0] = '\0';
    }
}

std::vector<filter_ui_state> get_filters(c_entity entity) {
    std::vector<filter_ui_state> filters;

    auto &door_man = component_system_man.managers.door_component_man;
    if (door_man.exists(entity)) {
        {
            auto door = door_man.get_instance_data(entity);
            add_filter(filters, 0, *(door.filter),
                "Door/filter");
        }
    }
    auto &gas_producer_man = component_system_man.managers.gas_producer_component_man;
    if (gas_producer_man.exists(entity)) {
        {
            auto gas_producer = gas_producer_man.get_instance_data(entity);
            add_filter(filters, 1, *(gas_producer.filter),
                "Gas Producer/filter");
        }
    }
    auto &light_man = component_system_man.managers.light_component_man;
    if (light_man.exists(entity)) {
        {
            auto light = light_man.get_instance_data(entity);
            add_filter(filters, 2, *(light.filter),
                "Light/filter");
        }
    }
    auto &rotator_man = component_system_man.managers.rotator_component_man;
    if (rotator_man.exists(entity)) {
        {
            auto rotator = rotator_man.get_instance_data(entity);
            add_filter(filters, 3, *(rotator.filter),
                "Rotator/filter");
        }
    }
    auto &sensor_comparator_man = component_system_man.managers.sensor_comparator_component_man;
    if (sensor_comparator_man.exists(entity)) {
        {
            auto sensor_comparator = sensor_comparator_man.get_instance_data(entity);
            add_filter(filters, 4, *(sensor_comparator.input_a),
                "Sensor Comparator/input_a");
        }
        {
            auto sensor_comparator = sensor_comparator_man.get_instance_data(entity);
            add_filter(filters, 5, *(sensor_comparator.input_b),
                "Sensor Comparator/input_b");
        }
    }
    return filters;
}

void update_filter(c_entity entity, filter_ui_state const& filter) {
    switch (filter.field_id) {
    case 0: {
        auto &door_man = component_system_man.managers.door_component_man;
        if (door_man.exists(entity)) {
            auto door = door_man.get_instance_data(entity);
            door.filter->set(filter);
        }
    } break;

    case 1: {
        auto &gas_producer_man = component_system_man.managers.gas_producer_component_man;
        if (gas_producer_man.exists(entity)) {
            auto gas_producer = gas_producer_man.get_instance_data(entity);
            gas_producer.filter->set(filter);
        }
    } break;

    case 2: {
        auto &light_man = component_system_man.managers.light_component_man;
        if (light_man.exists(entity)) {
            auto light = light_man.get_instance_data(entity);
            light.filter->set(filter);
        }
    } break;

    case 3: {
        auto &rotator_man = component_system_man.managers.rotator_component_man;
        if (rotator_man.exists(entity)) {
            auto rotator = rotator_man.get_instance_data(entity);
            rotator.filter->set(filter);
        }
    } break;

    case 4: {
        auto &sensor_comparator_man = component_system_man.managers.sensor_comparator_component_man;
        if (sensor_comparator_man.exists(entity)) {
            auto sensor_comparator = sensor_comparator_man.get_instance_data(entity);
            sensor_comparator.input_a->set(filter);
        }
    } break;

    case 5: {
        auto &sensor_comparator_man = component_system_man.managers.sensor_comparator_component_man;
        if (sensor_comparator_man.exists(entity)) {
            auto sensor_comparator = sensor_comparator_man.get_instance_data(entity);
            sensor_comparator.input_b->set(filter);
        }
    } break;

    }
}
