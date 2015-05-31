#pragma once

#include <unordered_map>

#include "input.h"
#include "settings.h"

void configure_bindings(std::unordered_map<en_action, action, std::hash<int>> &);

void configure_settings(settings &);

void configure_input_settings(input_settings &);

void configure_video_settings(video_settings &);
