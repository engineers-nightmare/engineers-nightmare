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

void
use_action_on_entity(ship_space *ship, c_entity ce);

/* todo: support free-placed entities*/
void
place_entity_attaches(raycast_info_block* rc, int index, c_entity e);
