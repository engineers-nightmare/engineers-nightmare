#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"
#include "../input.h"
#include "../settings.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;
extern player pl;
extern asset_manager asset_man;
extern en_settings game_settings;

struct wire_pos {
    glm::ivec3 pos;
    unsigned face;

    bool operator==(wire_pos const & other) const {
        return pos == other.pos && face == other.face;
    }

    bool operator!=(wire_pos const & other) const {
        return pos != other.pos || face != other.face;
    }

    struct hash {
        size_t operator()(wire_pos const &w) const {
            return ivec3_hash()(w.pos) ^ std::hash<unsigned>()(w.face);
        }
    };
};

static wire_pos from_rc(raycast_info_block const *rc) {
    return wire_pos{ rc->p, normal_to_surface_index(rc) ^ 1 };
}

template<typename T>
static void for_each_neighbor(wire_pos w, T const & f) {
    auto bl = ship->get_block(w.pos);
    for (auto i = 0u; i < 6u; i++) {
        // not the current face, and not the opposite one
        // (would require a span across the middle of the block)
        if (i != w.face && i != (w.face ^ 1)) {
            if (bl->surfs[i]) {
                // around an inside corner
                f({ w.pos, i });
            }
            else {
                // straight along the surface
                auto np = w.pos + surface_index_to_normal(i);
                auto n = ship->get_block(np);
                if (n && n->surfs[w.face]) {
                    f({ np, w.face });
                }
                else {
                    // outside corner
                    auto rp = np + surface_index_to_normal(w.face);
                    auto r = ship->get_block(rp);
                    if (r && r->surfs[i ^ 1]) {
                        f({ rp, i ^ 1 });
                    }
                }
            }
        }
    }
}

static float heuristic(wire_pos from, wire_pos to) {
    auto manhattan =
        abs(from.pos.x - to.pos.x) +
        abs(from.pos.y - to.pos.y) +
        abs(from.pos.z - to.pos.z);
    return (float)manhattan;
}

std::vector<wire_pos> find_path(wire_pos from, wire_pos to) {
    struct path_state {
        float g{ 0.f };
        wire_pos prev;
    };

    std::unordered_map<wire_pos, path_state, wire_pos::hash> state;
    state[from] = path_state{ 0.f, from };
    using workitem = std::tuple<float, wire_pos>;
    std::vector<workitem> worklist;
    worklist.emplace_back(0.f, from);
    std::vector<wire_pos> path;

    auto comp = [](workitem const &a, workitem const &b) {
        // compare priorities only.
        return std::get<0>(a) > std::get<0>(b);
    };

    while (!worklist.empty()) {
        auto w = worklist.front();
        std::pop_heap(worklist.begin(), worklist.end(), comp);
        worklist.pop_back();

        auto wp = std::get<1>(w);
        if (wp == to) {
            path.push_back(wp);
            while (wp != from) {
                wp = state[wp].prev;
                path.push_back(wp);
            }
            break;
        }

        auto &ws = state[wp];

        for_each_neighbor(wp, [&](wire_pos const &n) {
            auto cost = ws.g;
            if (!ship->get_block(n.pos)->has_wire[n.face]) cost += 1.f;
            auto is_new = state.find(n) == state.end();
            auto &ns = state[n];
            if (is_new || cost < ns.g) {
                auto f = cost + heuristic(n, to);
                worklist.emplace_back(f, n);
                std::push_heap(worklist.begin(), worklist.end(), comp);
                ns.prev = wp;
                ns.g = cost;
            }
        });
    }

    return path;    // no path.
}

struct wiring_tool : tool
{
    raycast_info_block rc;
    enum { idle, placing } state = idle;
    wire_pos start;
    wire_pos last_end;
    std::vector<wire_pos> path;
    int total_run = 0;
    int new_wire = 0;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, cross_surface, &rc);
    }

    bool can_use() {
        return rc.hit;
    }

    void use() override
    {
        if (!can_use())
            return;

        switch (state) {
        case idle:
            start = from_rc(&rc);
            state = placing;
            break;

        case placing: {
            if (path.size()) {
                for (auto &pe : path) {
                    ship->get_block(pe.pos)->has_wire[pe.face] = true;
                }
                state = idle;
            }
        } break;
        }
    }

    void alt_use() override
    {
        if (state == placing) {
            // cancel is always OK.
            state = idle;
            return;
        }

        if (!can_use())
            return;

        auto p = from_rc(&rc);
        ship->get_block(p.pos)->has_wire[p.face] = false;
    }

    void preview(frame_data *frame) override
    {
        auto p = from_rc(&rc);

        total_run = 0;
        new_wire = 0;

        // TODO: clean this mess up.
        if (state == placing) {
            if (last_end != p) {
                last_end = p;
                path = find_path(start, p);
                pl.ui_dirty = true;
            }

            auto mesh = asset_man.get_mesh("face_marker");

            if (!path.size()) {
                auto mat = frame->alloc_aligned<mesh_instance>(1);
                mat.ptr->world_matrix = mat_block_face(glm::vec3(start.pos), start.face);
                mat.ptr->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
                mat.bind(1, frame);

                glUseProgram(overlay_shader);
                draw_mesh(mesh.hw);
                glUseProgram(simple_shader);
                return;
            }

            glUseProgram(overlay_shader);
            for (auto & pe : path) {
                total_run++;
                if (!ship->get_block(pe.pos)->has_wire[pe.face]) {
                    auto mat = frame->alloc_aligned<mesh_instance>(1);
                    mat.ptr->world_matrix = mat_block_face(glm::vec3(pe.pos), pe.face);
                    mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
                    mat.bind(1, frame);
                    draw_mesh(mesh.hw);
                    new_wire++;
                }

                auto mesh2 = asset_man.get_surface_mesh(pe.face, surface_wall);
                auto mat = frame->alloc_aligned<mesh_instance>(1);
                mat.ptr->world_matrix = mat_position(glm::vec3(pe.pos));       // this is stupid.
                mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
                mat.bind(1, frame);
                draw_mesh(mesh2.hw);
            }

            glUseProgram(simple_shader);
        }

        if (!can_use())
            return; /* n/a */

        auto mesh = asset_man.get_mesh("face_marker");

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_block_face(glm::vec3(p.pos), p.face);
        mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat.bind(1, frame);

        glUseProgram(overlay_shader);
        draw_mesh(mesh.hw);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        auto bind = game_settings.bindings.bindings.find(action_alt_use_tool);
        auto alt_use = lookup_key((*bind).second.binds.inputs[0]);

        switch (state) {
        case idle:
            sprintf(str, "Place wiring   %s: Remove wiring", alt_use); break;
        case placing:
            if (total_run) {
                sprintf(str, "Finish run: %dm of new wire, total run %dm    %s: Cancel",
                    new_wire, total_run, alt_use);
            }
            else {
                sprintf(str, "Invalid placement!    %s: Remove wiring", alt_use);
            }
            break;
        }
    }

    void select() override { state = idle; }
};


tool *tool::create_wiring_tool() { return new wiring_tool(); }
