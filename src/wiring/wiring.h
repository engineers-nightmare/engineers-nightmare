#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../render_data.h"
#include "../component/component_manager.h"
#include "wiring_data.h"

struct ship_space;

struct wire_attachment {
    glm::mat4 transform;
    unsigned parent;
    unsigned rank;
    bool fixed;     /* this attachment is baked into an entity and cannot be moved/deleted by the player */
};

struct wire_segment {
    unsigned first;
    unsigned second;
};

static unsigned const invalid_attach = -1;
static unsigned const invalid_wire = -1;

unsigned
attach_topo_find(ship_space *ship, wire_type type, unsigned p);

void
calculate_power_wires(ship_space *ship);

void
propagate_comms_wires(ship_space *ship);

void
publish_msg(ship_space *ship, unsigned network, comms_msg msg);
