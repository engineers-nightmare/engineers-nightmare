#pragma once

#include "../common.h"
#include "../chunk.h"
#include "../mesh.h"
#include "../render_data.h"
#include "../ship_space.h"
#include "../player.h"

#include "component_managers.h"

struct component_system_manager {
    component_system_manager() = default;

    virtual ~component_system_manager() = default;

    component_managers managers{};
};

void
tick_gas_producers(ship_space *ship);

void
tick_doors(ship_space *ship);

void
tick_power_consumers(ship_space *ship);

void
tick_light_components(ship_space *ship);

void
tick_rotator_components(ship_space *ship, float dt);

void
tick_pressure_sensors(ship_space *ship);

void
tick_sensor_comparators(ship_space *ship);

void
tick_proximity_sensors(ship_space *ship, player *pl);

void
draw_renderables(frame_data *frame);

void
build_absolute_transforms();

template<typename T>
static inline
T load_value_from_config(config_setting_t const *s, char const *key) = delete;

template<>
float load_value_from_config<float>(config_setting_t const *s, char const *key) {
    auto m = config_setting_get_member(s, key);
    return (float)config_setting_get_float(m);
}

template<>
int load_value_from_config<int>(config_setting_t const *s, char const *key) {
    auto m = config_setting_get_member(s, key);
    return config_setting_get_int(m);
}

template<>
placement load_value_from_config<placement>(config_setting_t const *s, char const *key) {
    auto m = config_setting_get_member(s, key);
    return config_setting_get_placement(m);
}

template<>
std::string load_value_from_config<std::string>(config_setting_t const *s, char const *key) {
    auto m = config_setting_get_member(s, key);
    return config_setting_get_string(m);
}

template<>
glm::vec3 load_value_from_config<glm::vec3>(config_setting_t const *s, char const *key) {
    auto m = config_setting_get_member(s, key);
    float v[3]{};
    for (int i = 0; i < 3; i++) {
        v[i] = (float)config_setting_get_float_elem(m, i);
    }
    return glm::make_vec3(v);
}

template<>
glm::mat4 load_value_from_config<glm::mat4>(config_setting_t const *s, char const *key) {
    auto m = config_setting_get_member(s, key);
    float v[16]{};
    for (int i = 0; i < 16; i++) {
        v[i] = (float)config_setting_get_float_elem(m, i);
    }
    return glm::make_mat4(v);
}

