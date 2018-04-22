#pragma once

#include <libconfig.h>
#include "../libconfig_shim.h"

/* THIS FILE IS AUTOGENERATED BY gen/gen_enums.py; DO NOT HAND-MODIFY */


template<typename T>
    T get_enum(const char *e);


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
enum class msg_type
{
    any = 0,
    switch_transition = 1,
    pressure_sensor = 2,
    sensor_comparison = 3,
    proximity_sensor = 4,
    invalid = -1,
};

const char* get_enum_description(msg_type value);

const char* get_enum_string(msg_type value);

template<> msg_type get_enum<msg_type>(const char *e);

msg_type config_setting_get_msg_type(const config_setting_t *setting);

int config_setting_set_msg_type(config_setting_t *setting, msg_type value);

int config_setting_lookup_msg_type(const config_setting_t *setting, const char *name, msg_type *value);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
enum class placement
{
    full_block_snapped = 0,
    half_block_snapped = 1,
    quarter_block_snapped = 2,
    eighth_block_snapped = 3,
    invalid = -1,
};

const char* get_enum_description(placement value);

const char* get_enum_string(placement value);

template<> placement get_enum<placement>(const char *e);

placement config_setting_get_placement(const config_setting_t *setting);

int config_setting_set_placement(config_setting_t *setting, placement value);

int config_setting_lookup_placement(const config_setting_t *setting, const char *name, placement *value);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
enum class window_mode
{
    windowed = 0,
    fullscreen = 1,
    invalid = -1,
};

const char* get_enum_description(window_mode value);

const char* get_enum_string(window_mode value);

template<> window_mode get_enum<window_mode>(const char *e);

window_mode config_setting_get_window_mode(const config_setting_t *setting);

int config_setting_set_window_mode(config_setting_t *setting, window_mode value);

int config_setting_lookup_window_mode(const config_setting_t *setting, const char *name, window_mode *value);
