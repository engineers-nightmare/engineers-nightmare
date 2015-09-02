#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "render_data.h"

struct wire_attachment {
    glm::mat4 transform;
};

struct wire_segment {
    unsigned first = -1;
    unsigned second = -1;
};

struct wire {
    std::vector<wire_segment> segments;
};


bool
segment_finished(wire_segment* segment);

void
draw_attachments(frame_data *frame);
