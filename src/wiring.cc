#include "wiring.h"
#include "mesh.h"


sw_mesh *attachment_sw;
hw_mesh *attachment_hw;
sw_mesh *no_placement_sw;
hw_mesh *no_placement_hw;
sw_mesh *wire_sw;
hw_mesh *wire_hw;


std::vector<wire_attachment> wire_attachments;
std::vector<wire_segment> wire_segments;


extern glm::mat4
mat_scale(glm::vec3 scale);

extern glm::mat4
mat_scale(float x, float y, float z);

extern glm::mat4
mat_rotate_mesh(glm::vec3 pt, glm::vec3 dir);


void
draw_attachments(frame_data *frame)
{
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


glm::mat4
calc_segment_matrix(const wire_attachment &start, const wire_attachment &end) {
    auto a1 = start.transform;
    auto a2 = end.transform;

    auto p1_4 = a1[3];
    auto p2_4 = a2[3];

    auto p1 = glm::vec3(p1_4.x, p1_4.y, p1_4.z);
    auto p2 = glm::vec3(p2_4.x, p2_4.y, p2_4.z);

    auto seg = p1 - p2;
    auto len = glm::length(seg);

    auto scale = mat_scale(1, 1, len);

    auto rot = mat_rotate_mesh(p2, glm::normalize(p2 - p1));

    return rot * scale;
}


void
draw_segments(frame_data *frame)
{
    auto count = wire_segments.size();
    for (auto i = 0u; i < count; i += INSTANCE_BATCH_SIZE) {
        auto batch_size = std::min(INSTANCE_BATCH_SIZE, (unsigned)(count - i));
        auto segment_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

        for (auto j = 0u; j < batch_size; j++) {
            auto segment = wire_segments[i + j];

            auto a1 = wire_attachments[segment.first];
            auto a2 = wire_attachments[segment.second];

            auto mat = calc_segment_matrix(a1, a2);

            segment_matrices.ptr[j] = mat;
        }

        segment_matrices.bind(1, frame);
        draw_mesh_instanced(wire_hw, batch_size);
    }
}


unsigned
attach_topo_find(unsigned p)
{
    /* compress paths as we go */
    if (wire_attachments[p].parent != p) {
        wire_attachments[p].parent = attach_topo_find(wire_attachments[p].parent);
    }

    return wire_attachments[p].parent;
}


unsigned
attach_topo_unite(unsigned from, unsigned to)
{
    /* merge together trees containing two attaches */
    from = attach_topo_find(from);
    to = attach_topo_find(to);

    /* already in same subtree? */
    if (from == to) {
        return from;
    }

    if (wire_attachments[from].rank < wire_attachments[to].rank) {
        wire_attachments[from].parent = to;
        return to;
    }
    else if (wire_attachments[from].rank > wire_attachments[to].rank) {
        wire_attachments[to].parent = from;
        return from;
    }
    else {
        /* two rank-n trees merge to form a rank-n+1 tree. the choice of
         * root is arbitrary
         */
        wire_attachments[to].parent = from;
        wire_attachments[from].rank++;
        return from;
    }
}


/* not currently used, but we'll need it once there is deletion of attaches
 * or segments.
 */
void
attach_topo_rebuild(void)
{
    /* 1. everything points to itself, with rank 0 */
    auto count = wire_attachments.size();
    for (auto i = 0u; i < count; i++) {
        wire_attachments[i].parent = i;
        wire_attachments[i].rank = 0;
    }

    /* 2. walk all the segments, unifying */
    for (auto & seg : wire_segments) {
        attach_topo_unite(seg.first, seg.second);
    }
}
