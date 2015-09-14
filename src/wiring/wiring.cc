#include "wiring.h"
#include "../mesh.h"
#include "../ship_space.h"
#include "../component/component_system_manager.h"

sw_mesh *attachment_sw;
hw_mesh *attachment_hw;
sw_mesh *no_placement_sw;
hw_mesh *no_placement_hw;
sw_mesh *wire_sw;
hw_mesh *wire_hw;


extern glm::mat4
mat_scale(glm::vec3 scale);

extern glm::mat4
mat_scale(float x, float y, float z);

extern glm::mat4
mat_rotate_mesh(glm::vec3 pt, glm::vec3 dir);


void
draw_attachments(ship_space *ship, frame_data *frame)
{
    for (auto const & wire_attachments : ship->wire_attachments) {
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
    auto const num_wires = sizeof(ship->wire_attachments) / sizeof(ship->wire_attachments[0]);
    for (auto index = 0u; index < num_wires; ++index) {
        auto const & wire_attachments = ship->wire_attachments[index];
        auto const & wire_segments = ship->wire_segments[index];

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
}


bool
remove_segments_containing(ship_space *ship, wire_type type, unsigned attach) {
    /* remove all segments that contain attach */
    auto changed = false;
    auto & wire_segments = ship->wire_segments[type];
    for (auto si = wire_segments.begin(); si != wire_segments.end(); ) {
        if (si->first == attach || si->second == attach) {
            si = wire_segments.erase(si);
            changed = true;
        }
        else {
            ++si;
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
    for (size_t i1 = 0; i1 < wire_segments.size(); ++i1) {
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

    ship->power_wires.clear();

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
calculate_power(ship_space *ship) {
    /* get all wires
     * for each wire
     * go through all entities
     * add power data to corresponding wire
     */
    ship->power_wires.clear();
    const auto type = wire_type_power;
    for (auto const & attach : ship->power_attachments) {
        auto wire = attach_topo_find(ship, type, attach.parent);
        if (ship->power_wires.find(wire) == ship->power_wires.end()) {
            continue;
        }
        ship->power_wires[wire] = power_wiring_data();
    }

    /* only visit each wire once.
     * They should all be flat with attach_topo_find()
     */
    std::set<size_t> visited_wires;
    std::set<c_entity> visited_entities;
    for (auto const & attach : ship->power_attachments) {
        auto wire = attach_topo_find(ship, type, attach.parent);
        if (visited_wires.find(wire) != visited_wires.end()) {
            continue;
        }

        visited_wires.insert(wire);
        visited_entities.clear();

        for (auto const & entity_lookup : ship->entity_to_attach_lookups[type]) {
            auto const & entity = entity_lookup.first;
            if (visited_entities.find(entity) != visited_entities.end()) {
                continue;
            }

            visited_entities.insert(entity_lookup.first);
            for (auto const & ea_index : entity_lookup.second) {
                auto const & ea = ship->wire_attachments[type][ea_index];
                if (attach_topo_find(ship, type, ea.parent) == wire) {
                    auto & power_data = ship->power_wires[wire];
                    /* add to power_data on wire */
                    if (power_man.exists(entity)) {
                        power_data.num_consumers++;
                        power_data.total_draw += power_man.required_power(entity);
                    }
                    if (power_provider_man.exists(entity)) {
                        power_data.num_providers++;
                        power_data.total_power += power_provider_man.provided(entity);
                    }
                    break;
                }
            }
        }
    }
}
