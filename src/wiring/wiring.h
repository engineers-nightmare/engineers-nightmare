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
static unsigned const invalid_wire = -1;

void
draw_attachments(ship_space *ship, frame_data *frame);

void
draw_attachments_on_active_wire(ship_space *ship, frame_data *frame);

void
draw_segments(ship_space *ship, frame_data *frame);

void
draw_active_segments(ship_space *ship, frame_data *frame);

bool
remove_segments_containing(ship_space *ship, wire_type type, unsigned attach);

bool
relocate_single_attach(ship_space *ship, wire_type type,
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
propagate_comms_wires(ship_space *ship);

void
publish_msg(ship_space *ship, c_entity ce, comms_msg msg);

void
remove_attaches_for_entity(ship_space *ship, c_entity ce);
