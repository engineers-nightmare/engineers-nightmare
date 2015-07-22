#pragma once

#include <unordered_map>

#include "input.h"
#include "settings.h"

enum en_config_type {
    en_config_base,
    en_config_user,
};


en_settings load_settings(en_config_type);
input_settings load_input_settings(en_config_type);

binding_settings load_binding_settings(en_config_type);

video_settings load_video_settings(en_config_type);

en_settings merge_settings(en_settings, en_settings);
