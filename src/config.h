#pragma once

#include <unordered_map>

#include "input.h"
#include "settings.h"

enum en_config_type {
    en_config_base,
    en_config_user,
};

void save_settings(const en_settings &);
void save_binding_settings(const binding_settings &);
void save_audio_settings(const audio_settings &);
void save_video_settings(const video_settings &);
void save_input_settings(const input_settings &);

en_settings load_settings(const en_config_type &);
input_settings load_input_settings(const en_config_type &);
binding_settings load_binding_settings(const en_config_type &);
audio_settings load_audio_settings(const en_config_type &);
video_settings load_video_settings(const en_config_type &);

en_settings merge_settings(const en_settings &, const en_settings &);
