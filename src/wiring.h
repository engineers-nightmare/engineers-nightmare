#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "render_data.h"

struct wire_attachment {
    glm::mat4 transform;
    unsigned parent;
};

struct wire_segment {
    unsigned first;
    unsigned second;
};

void
draw_attachments(frame_data *frame);

void
draw_segments(frame_data *frame);

glm::mat4
calc_segment_matrix(const wire_attachment &start, const wire_attachment &end);
