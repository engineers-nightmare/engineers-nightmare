#include <epoxy/gl.h>
#include <glm/ext.hpp>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern std::unordered_map<std::string, ::mesh_data> meshes;
extern std::array<std::string, face_count> surface_index_to_mesh;

bool
paint_surface_tool::can_use(const raycast_info *rc) const {
    auto *block = rc->block;
    auto bl = rc->bl;
    auto index = normal_to_surface_index(rc);
    auto si = (surface_index)index;
    auto *other = ship->get_block(rc->p);

    // if we've started, only allow same plane
    if (state == paint_state::started) {
        if (other->type != block_empty) {
            return false;
        }

        switch ((surface_index)index) {
            case surface_xp:
            case surface_xm:
                if (start_block.x != bl.x && start_index != si)
                    return false;
                break;
            case surface_yp:
            case surface_ym:
                if (start_block.y != bl.y && start_index != si)
                    return false;
                break;
            case surface_zp:
            case surface_zm:
                if (start_block.z != bl.z && start_index != si)
                    return false;
                break;
            case face_count:
                assert(false);
                return false;
                break;
        }
    }

    return (block && block->type == block_frame);
}

void
paint_surface_tool::use(raycast_info *rc) {
    if (!rc->hit)
        return;

    int index = normal_to_surface_index(rc);
    auto si = (surface_index)index;

    if (can_use(rc)) {
        switch (state) {
            case paint_state::started: {
                finish_paint(rc->bl);

                state = paint_state::idle;
                break;
            }
            case paint_state::idle: {
                start_block = rc->bl;
                start_index = si;
                select_type = rc->block->surfs[index];

                state = paint_state::started;
                break;
            }
        }
    }
}


void
paint_surface_tool::finish_paint(glm::ivec3 end) {
    auto cur = start_block;

    int i[2] = {0, 0};
    if (start_index == surface_xm || start_index == surface_xp) {
        i[0] = 1;
        i[1] = 2;
    }
    else if (start_index == surface_ym || start_index == surface_yp) {
        i[0] = 0;
        i[1] = 2;
    }
    else {
        i[0] = 0;
        i[1] = 1;
    }

    auto min = glm::min(start_block, end);
    auto max = glm::max(start_block, end);

    auto s = glm::value_ptr(min);
    auto e = glm::value_ptr(max);
    auto c = glm::value_ptr(cur);
    for (auto j = s[i[0]]; j <= e[i[0]]; j++) {
        for (auto k = s[i[1]]; k <= e[i[1]]; k++) {
            c[i[0]] = j;
            c[i[1]] = k;

            auto block = ship->get_block(cur);
            if (mode == replace_mode::match && block->surfs[start_index] != select_type) {
                continue;
            }

            auto o = cur + surface_index_to_normal(start_index);
            ship->set_surface(cur, o, start_index, replace_type);
        }
    }
}


void
paint_surface_tool::alt_use(raycast_info *rc) {
    switch (mode) {
        case replace_mode::all:
            mode = replace_mode::match;
            break;
        case replace_mode::match:
            mode = replace_mode::all;
            break;
    }
}


void
paint_surface_tool::long_use(raycast_info *rc) {
    state = paint_state::idle;
}


void
paint_surface_tool::cycle_mode() {
    switch (replace_type) {
    case surface_wall:
        replace_type = surface_grate;
        break;
    case surface_grate:
        replace_type = surface_glass;
        break;
    case surface_glass:
        replace_type = surface_wall;
        break;
    default:
        replace_type = surface_wall;
        break;
    }
}


void
paint_surface_tool::preview(raycast_info *rc, frame_data *frame) {
    int index = normal_to_surface_index(rc);

    if (state == paint_state::started) {
        auto mat = frame->alloc_aligned<glm::mat4>(1);
        *mat.ptr = mat_position(start_block);
        mat.bind(1, frame);

        auto mesh = meshes[surface_index_to_mesh[index]];

        glUseProgram(remove_overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(mesh.hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    if (!rc->hit)
        return;

    if (can_use(rc)) {
        switch (state) {
            case paint_state::started: {
                auto cur = start_block;

                int i[2] = {0, 0};
                if (start_index == surface_xm || start_index == surface_xp) {
                    i[0] = 1;
                    i[1] = 2;
                } else if (start_index == surface_ym || start_index == surface_yp) {
                    i[0] = 0;
                    i[1] = 2;
                } else {
                    i[0] = 0;
                    i[1] = 1;
                }

                auto min = glm::min(start_block, rc->bl);
                auto max = glm::max(start_block, rc->bl);

                auto s = glm::value_ptr(min);
                auto e = glm::value_ptr(max);
                auto c = glm::value_ptr(cur);
                for (auto j = s[i[0]]; j <= e[i[0]]; j++) {
                    for (auto k = s[i[1]]; k <= e[i[1]]; k++) {
                        c[i[0]] = j;
                        c[i[1]] = k;
                        auto block = ship->get_block(cur);
                        if (mode == replace_mode::match && block->surfs[start_index] != select_type) {
                            continue;
                        }

                        auto mat = frame->alloc_aligned<glm::mat4>(1);
                        *mat.ptr = mat_position(cur);
                        mat.bind(1, frame);

                        auto mesh = meshes[surface_index_to_mesh[index]];

                        glUseProgram(remove_overlay_shader);
                        glEnable(GL_POLYGON_OFFSET_FILL);
                        draw_mesh(mesh.hw);
                        glDisable(GL_POLYGON_OFFSET_FILL);
                        glUseProgram(simple_shader);
                    }
                }

                break;
            }
            case paint_state::idle: {
                auto mat = frame->alloc_aligned<glm::mat4>(1);
                *mat.ptr = mat_position(rc->bl);
                mat.bind(1, frame);

                auto mesh = meshes[surface_index_to_mesh[index]];

                glUseProgram(add_overlay_shader);
                glEnable(GL_POLYGON_OFFSET_FILL);
                draw_mesh(mesh.hw);
                glDisable(GL_POLYGON_OFFSET_FILL);
                glUseProgram(simple_shader);

                break;
            }
        }
    }
}


void
paint_surface_tool::get_description(char *str) {
    auto m = mode == replace_mode::all ? "all" : "matching";
    sprintf(str, "Paint surface type %d replacing %s\n", (int)replace_type, m);
}

void
paint_surface_tool::select() {
    state = paint_state::idle;
}


void
paint_surface_tool::unselect() {
    state = paint_state::idle;
}
