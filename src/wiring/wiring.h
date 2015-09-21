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
};

struct wire_segment {
    unsigned first;
    unsigned second;
};

static unsigned const invalid_attach = -1;

void
draw_attachments(ship_space *ship, frame_data *frame);

void
draw_segments(ship_space *ship, frame_data *frame);

bool
remove_segments_containing(ship_space *ship, wire_type type, unsigned attach);

bool
relocate_segments_and_entity_attaches(ship_space *ship, wire_type type,
    unsigned relocated_to, unsigned moved_from);

void
reduce_segments(ship_space *ship, wire_type type);

glm::mat4
calc_segment_matrix(const wire_attachment &start, const wire_attachment &end);

unsigned
attach_topo_find(ship_space *ship, wire_type type, unsigned p);

unsigned
attach_topo_unite(ship_space *ship, wire_type type, unsigned from, unsigned to);

void
attach_topo_rebuild(ship_space *ship, wire_type type);

void
calculate_power_wires(ship_space *ship);

void
calculate_comms_wires(ship_space *ship);

void
propagate_comms_wires(ship_space *ship);

void
publish_message(ship_space *ship, unsigned wire_id, comms_msg msg);
