#pragma once

/* THIS FILE IS AUTOGENERATED BY gen/gen_component_impl.py; DO NOT HAND-MODIFY */

#include <unordered_map>
#include <memory>

#include "component_manager.h"

#include "display_component.h"
#include "door_component.h"
#include "gas_producer_component.h"
#include "light_component.h"
#include "physics_component.h"
#include "power_component.h"
#include "power_provider_component.h"
#include "pressure_sensor_component.h"
#include "proximity_sensor_component.h"
#include "reader_component.h"
#include "relative_position_component.h"
#include "renderable_component.h"
#include "sensor_comparator_component.h"
#include "surface_attachment_component.h"
#include "switch_component.h"
#include "type_component.h"
#include "wire_comms_component.h"

#define INITIAL_MAX_COMPONENTS 20

struct component_managers {
    component_managers() {
        display_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        door_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        gas_producer_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        light_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        physics_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        power_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        power_provider_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        pressure_sensor_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        proximity_sensor_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        reader_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        relative_position_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        renderable_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        sensor_comparator_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        surface_attachment_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        switch_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        type_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
        wire_comms_component_man.create_component_instance_data(INITIAL_MAX_COMPONENTS);
    }

    display_component_manager display_component_man{};
    door_component_manager door_component_man{};
    gas_producer_component_manager gas_producer_component_man{};
    light_component_manager light_component_man{};
    physics_component_manager physics_component_man{};
    power_component_manager power_component_man{};
    power_provider_component_manager power_provider_component_man{};
    pressure_sensor_component_manager pressure_sensor_component_man{};
    proximity_sensor_component_manager proximity_sensor_component_man{};
    reader_component_manager reader_component_man{};
    relative_position_component_manager relative_position_component_man{};
    renderable_component_manager renderable_component_man{};
    sensor_comparator_component_manager sensor_comparator_component_man{};
    surface_attachment_component_manager surface_attachment_component_man{};
    switch_component_manager switch_component_man{};
    type_component_manager type_component_man{};
    wire_comms_component_manager wire_comms_component_man{};

    std::unique_ptr<component_stub> get_stub(const char*comp_name, const config_setting_t *config) {
        if (strcmp(comp_name, "display") == 0) {
            return display_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "door") == 0) {
            return door_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "gas_producer") == 0) {
            return gas_producer_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "light") == 0) {
            return light_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "physics") == 0) {
            return physics_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "power") == 0) {
            return power_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "power_provider") == 0) {
            return power_provider_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "pressure_sensor") == 0) {
            return pressure_sensor_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "proximity_sensor") == 0) {
            return proximity_sensor_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "reader") == 0) {
            return reader_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "relative_position") == 0) {
            return relative_position_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "renderable") == 0) {
            return renderable_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "sensor_comparator") == 0) {
            return sensor_comparator_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "surface_attachment") == 0) {
            return surface_attachment_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "switch") == 0) {
            return switch_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "type") == 0) {
            return type_component_stub::from_config(config);
        }
        if (strcmp(comp_name, "wire_comms") == 0) {
            return wire_comms_component_stub::from_config(config);
        }
        assert(false);
        return nullptr;
    }

    void destroy_entity_instance(c_entity ce) {
        display_component_man.destroy_entity_instance(ce);
        door_component_man.destroy_entity_instance(ce);
        gas_producer_component_man.destroy_entity_instance(ce);
        light_component_man.destroy_entity_instance(ce);
        physics_component_man.destroy_entity_instance(ce);
        power_component_man.destroy_entity_instance(ce);
        power_provider_component_man.destroy_entity_instance(ce);
        pressure_sensor_component_man.destroy_entity_instance(ce);
        proximity_sensor_component_man.destroy_entity_instance(ce);
        reader_component_man.destroy_entity_instance(ce);
        relative_position_component_man.destroy_entity_instance(ce);
        renderable_component_man.destroy_entity_instance(ce);
        sensor_comparator_component_man.destroy_entity_instance(ce);
        surface_attachment_component_man.destroy_entity_instance(ce);
        switch_component_man.destroy_entity_instance(ce);
        type_component_man.destroy_entity_instance(ce);
        wire_comms_component_man.destroy_entity_instance(ce);
    }
};
