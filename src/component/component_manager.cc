//
// Created by caleb on 11/1/17.
//
#include "component_manager.h"

std::unordered_map<std::string, std::function<component_stub(config_setting_t *)>> component_stub_generators;