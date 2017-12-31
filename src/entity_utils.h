//
// Created by caleb on 11/23/17.
//
#pragma once

#include <glm/glm.hpp>
#include <string>

#include "component/c_entity.h"
#include "common.h"
#include "ship_space.h"

void
load_entities();

c_entity
spawn_entity(const std::string &name, glm::ivec3 p, int face, glm::mat4 mat);

c_entity
spawn_unplaceable_entity(const std::string &name, glm::mat4 world);

c_entity
spawn_floating_generic_entity(glm::mat4 mat, const std::string &mesh, const std::string &phys_mesh);

void
pop_entity_off(c_entity entity);

void
use_action_on_entity(ship_space *ship, c_entity ce);
