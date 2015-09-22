#include <unordered_set>

#include "../common.h"
#include "wiring.h"
#include "../mesh.h"
#include "../ship_space.h"
#include "../component/component_system_manager.h"

sw_mesh *attachment_sw;
hw_mesh *attachment_hw;
sw_mesh *no_placement_sw;
hw_mesh *no_placement_hw;

hw_mesh *wire_hw_meshes[num_wire_types];


void
draw_attachments(ship_space *ship, frame_data *frame)
{
    for (auto type = 0u; type < num_wire_types; ++type) {
        auto const & wire_attachments = ship->wire_attachments[type];
        auto count = wire_attachments.size();
        for (auto i = 0u; i < count; i += INSTANCE_BATCH_SIZE) {
            auto drawn_attaches = 0u;
            auto batch_size = std::min(INSTANCE_BATCH_SIZE, (unsigned)(count - i));
            auto attachment_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

            for (auto j = 0u; j < batch_size; j++) {
                auto const & attach = wire_attachments[i + j];
                auto const & active_wires = ship->active_wire[type];

                auto wire = attach_topo_find(ship, (wire_type)type, attach.parent);

                if (wire == active_wires[0] || wire == active_wires[1]) {
                    continue;
                }

                attachment_matrices.ptr[drawn_attaches] = attach.transform;

                ++drawn_attaches;
            }

            attachment_matrices.bind(1, frame);
            draw_mesh_instanced(attachment_hw, drawn_attaches);
        }
    }
}


void
draw_attachments_on_active_wire(ship_space *ship, frame_data *frame)
{
    for (auto type = 0u; type < num_wire_types; ++type) {
        auto const & wire_attachments = ship->wire_attachments[type];
        auto count = wire_attachments.size();
        for (auto i = 0u; i < count; i += INSTANCE_BATCH_SIZE) {
            auto drawn_attaches = 0u;
            auto batch_size = std::min(INSTANCE_BATCH_SIZE, (unsigned)(count - i));
            auto attachment_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

            for (auto j = 0u; j < batch_size; j++) {
                auto const & attach = wire_attachments[i + j];
                auto const & active_wires = ship->active_wire[type];

                auto wire = attach_topo_find(ship, (wire_type)type, attach.parent);

                if (wire != active_wires[0] && wire != active_wires[1]) {
                    continue;
                }

                attachment_matrices.ptr[drawn_attaches] = attach.transform;

                ++drawn_attaches;
            }

            attachment_matrices.bind(1, frame);
            draw_mesh_instanced(attachment_hw, drawn_attaches);
        }
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
draw_segments(ship_space *ship, frame_data *frame) {
    for (auto type = 0u; type < num_wire_types; ++type) {
        auto const & wire_attachments = ship->wire_attachments[type];
        auto const & wire_segments = ship->wire_segments[type];
        auto active_wires = ship->active_wire[type];

        auto count = wire_segments.size();
        for (auto i = 0u; i < count; i += INSTANCE_BATCH_SIZE) {
            auto drawn_segments = 0u;
            auto batch_size = std::min(INSTANCE_BATCH_SIZE, (unsigned)(count - i));
            auto segment_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

            for (auto j = 0u; j < batch_size; j++) {
                auto segment = wire_segments[i + j];

                auto a1 = wire_attachments[segment.first];
                auto a2 = wire_attachments[segment.second];

                auto mat = calc_segment_matrix(a1, a2);

                auto wire = attach_topo_find(ship, (wire_type)type, a1.parent);
                if (wire == active_wires[0] || wire == active_wires[1]) {
                    continue;
                }

                segment_matrices.ptr[drawn_segments] = mat;

                ++drawn_segments;
            }

            segment_matrices.bind(1, frame);
            draw_mesh_instanced(wire_hw_meshes[type], drawn_segments);
        }
    }
}


void
draw_active_segments(ship_space *ship, frame_data *frame) {
    for (auto type = 0u; type < num_wire_types; ++type) {
        auto const & wire_attachments = ship->wire_attachments[type];
        auto const & wire_segments = ship->wire_segments[type];
        auto active_wires = ship->active_wire[type];

        if (active_wires[0] == invalid_wire && active_wires[1] == invalid_wire) {
            continue;
        }

        auto count = wire_segments.size();
        for (auto i = 0u; i < count; i += INSTANCE_BATCH_SIZE) {
            auto drawn_segments = 0u;
            auto batch_size = std::min(INSTANCE_BATCH_SIZE, (unsigned)(count - i));
            auto segment_matrices = frame->alloc_aligned<glm::mat4>(batch_size);

            for (auto j = 0u; j < batch_size; j++) {
                auto segment = wire_segments[i + j];

                auto a1 = wire_attachments[segment.first];
                auto a2 = wire_attachments[segment.second];

                auto wire = attach_topo_find(ship, (wire_type)type, a1.parent);
                if (wire != active_wires[0] && wire != active_wires[1]) {
                    continue;
                }

                auto mat = calc_segment_matrix(a1, a2);

                segment_matrices.ptr[drawn_segments] = mat;

                ++drawn_segments;
            }

            segment_matrices.bind(1, frame);
            draw_mesh_instanced(wire_hw_meshes[type], drawn_segments);
        }
    }
}


bool
remove_segments_containing(ship_space *ship, wire_type type, unsigned attach) {
    /* remove all segments that contain attach. relocates segments from
     * the end to avoid having to shuffle everything down. this is safe
     * without any further fixups -- nobody refers to segments /across/ this call
     * by index. */
    auto changed = false;
    auto & wire_segments = ship->wire_segments[type];
    for (auto i = 0u; i < wire_segments.size(); ) {
        if (wire_segments[i].first == attach || wire_segments[i].second == attach) {
            wire_segments[i] = wire_segments.back();
            wire_segments.pop_back();
            changed = true;
        }
        else {
            ++i;
        }
    }
    return changed;
}


bool
relocate_segments_and_entity_attaches(ship_space *ship, wire_type type,
    unsigned relocated_to, unsigned moved_from) {
    /* fixup segments with attaches that were relocated */
    auto & wire_segments = ship->wire_segments[type];
    auto changed = false;

    for (auto si = wire_segments.begin(); si != wire_segments.end(); ++si) {
        if (si->first == moved_from) {
            si->first = relocated_to;
            changed = true;
        }

        if (si->second == moved_from) {
            si->second = relocated_to;
            changed = true;
        }
    }

    /* fixup entity attaches that were relocated */
    for (auto& sea : ship->entity_to_attach_lookups[type]) {
        auto & sea_attaches = sea.second;
        if (sea_attaches.erase(moved_from)) {
            sea_attaches.insert(relocated_to);
        }
    }

    return changed;
}


void
reduce_segments(ship_space *ship, wire_type type) {
    auto & wire_segments = ship->wire_segments[type];
    for (auto i1 = 0u; i1 < wire_segments.size();) {
        auto remove = false;

        auto const & seg1 = wire_segments[i1];
        auto s1f = seg1.first;
        auto s1s = seg1.second;

        /* segment is attached to only one attach */
        if (s1f == s1s) {
            remove = true;
        }

        if (!remove) {
            for (auto i2 = i1 + 1; i2 < wire_segments.size(); ++i2) {
                auto const & seg2 = wire_segments[i2];
                auto s2f = seg2.first;
                auto s2s = seg2.second;

                /* segments share attaches on both sides */
                if ((s1f == s2f && s1s == s2s) || (s1f == s2s && s1s == s2f)) {
                    remove = true;
                    break;
                }
            }
        }

        if (remove) {
            wire_segments[i1] = wire_segments.back();
            wire_segments.pop_back();
        }
        else {
            i1++;
        }
    }
}


unsigned
attach_topo_find(ship_space *ship, wire_type type, unsigned p) {
    auto & wire_attachments = ship->wire_attachments[type];

    /* compress paths as we go */
    if (wire_attachments[p].parent != p) {
        wire_attachments[p].parent = attach_topo_find(ship, type, wire_attachments[p].parent);
    }

    return wire_attachments[p].parent;
}


unsigned
attach_topo_unite(ship_space *ship, wire_type type, unsigned from, unsigned to) {
    auto & wire_attachments = ship->wire_attachments[type];

    /* merge together trees containing two attaches */
    from = attach_topo_find(ship, type, from);
    to = attach_topo_find(ship, type, to);

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


void
attach_topo_rebuild(ship_space *ship, wire_type type) {
    auto &wire_attachments = ship->wire_attachments[type];
    auto &wire_segments = ship->wire_segments[type];

    /* 1. everything points to itself, with rank 0 */
    auto count = wire_attachments.size();
    for (auto i = 0u; i < count; i++) {
        wire_attachments[i].parent = i;
        wire_attachments[i].rank = 0;
    }

    /* 2. walk all the segments, unifying */
    for (auto const & seg : wire_segments) {
        attach_topo_unite(ship, type, seg.first, seg.second);
    }
}


/* calculates power data for each power run
 * assumes a rebuilt attach topo
 */
void
calculate_power_wires(ship_space *ship) {
    ship->power_wires.clear();
    const auto type = wire_type_power;

    /* note: power_wires operator[] default-constructs the value if not present. */

    /* walk power consumers */
    /* invariant: at most /one/ power wire is attached. */
    for (auto i = 0u; i < power_man.buffer.num; i++) {
        auto ce = power_man.instance_pool.entity[i];
        auto attaches = ship->entity_to_attach_lookups[type].find(ce);
        if (attaches != ship->entity_to_attach_lookups[type].end()) {
            for (auto attach : attaches->second) {
                auto wire = attach_topo_find(ship, type, attach);
                auto & power_data = ship->power_wires[wire];

                auto power_draw_if_enabled = power_man.instance_pool.required_power[i];

                power_data.num_consumers++;
                power_data.peak_draw += power_draw_if_enabled;

                if (!switchable_man.exists(ce) || switchable_man.enabled(ce)) {
                    power_data.total_draw += power_draw_if_enabled;
                }
            }
        }
    }

    /* walk power producers */
    /* invariant: at most /one/ power wire is attached. */
    for (auto i = 0u; i < power_provider_man.buffer.num; i++) {
        auto ce = power_provider_man.instance_pool.entity[i];
        auto attaches = ship->entity_to_attach_lookups[type].find(ce);
        if (attaches != ship->entity_to_attach_lookups[type].end()) {
            for (auto attach : attaches->second) {
                auto wire = attach_topo_find(ship, type, attach);
                auto & power_data = ship->power_wires[wire];

                power_data.total_power += power_provider_man.instance_pool.provided[i];
                power_data.num_providers++;
            }
        }
    }
}

/* Phase boundary: this frame's written messages become the next frame's messages to read.
 * start off the next frame's written messages as empty.
 *
 * This allows any tick code to read or write messages however it likes, but never step on each
 * other or introduce strange order-based inconsistencies.
 */
void
propagate_comms_wires(ship_space *ship) {
    for (auto & w : ship->comms_wires) {
        std::swap(w.second.read_buffer, w.second.write_buffer);
        w.second.write_buffer.clear();
    }
}

void
publish_message(ship_space* ship, unsigned wire_id, comms_msg msg) {
    if (ship->comms_wires.find(wire_id) == ship->comms_wires.end()) {
        return;
    }

    auto & wire = ship->comms_wires[wire_id];
    wire.write_buffer.push_back(msg);
}
