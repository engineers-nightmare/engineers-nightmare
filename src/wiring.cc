#include "wiring.h"

#include "mesh.h"

hw_mesh *attachment_hw;
sw_mesh *attachment_sw;

std::vector<wire_attachment> wire_attachments;
std::vector<wire> wires;

bool segment_finished(wire_segment* segment) {
    if (!segment) {
        return true;
    }

    return segment->first != ~0u && segment->second != ~0u;
}

void
draw_attachments(frame_data *frame) {
    auto count = wire_attachments.size();
    for (auto i = 0u; i < count; i += INSTANCE_BATCH_SIZE) {
        auto batch_size = std::min(INSTANCE_BATCH_SIZE, (unsigned)(count - i));
        auto attachment_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

        for (auto j = 0u; j < batch_size; j++) {
            attachment_matrices.ptr[j] = wire_attachments[i + j].transform;
        }

        attachment_matrices.bind(1, frame);
        draw_mesh_instanced(attachment_hw, batch_size);
    }
}
