#pragma once

#include <glm/glm.hpp>
#include <string>

#include "component/c_entity.h"
#include "common.h"
#include "ship_space.h"

void
load_entities();

c_entity
spawn_entity(const std::string &name, glm::mat4 mat);

c_entity
spawn_floating_generic_entity(glm::mat4 mat, const std::string &mesh, const std::string &phys_mesh);

void
attach_entity_to_surface(c_entity ce, glm::ivec3 p, int face);

void
destroy_entity(c_entity e);

void
pop_entity_off(c_entity entity);

void
use_action_on_entity(ship_space *ship, c_entity ce);
