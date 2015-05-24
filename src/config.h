#pragma once

#include <unordered_map>

#include "input.h"

void configureBindings(std::unordered_map<en_action, action, std::hash<int>> &);
