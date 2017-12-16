#include <epoxy/gl.h>
#include <glm/ext.hpp>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "../player.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;


struct paint_surface_tool : tool
{
    surface_type select_type = surface_wall;
    surface_type replace_type = surface_wall;

    glm::ivec3 start_block;
    surface_index start_index;
    raycast_info_block rc;

    enum class replace_mode {
        all,
        match,
    } mode = replace_mode::match;

    enum class paint_state {
        started,
        idle,
    } state = paint_state::idle;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, 
            (raycast_stopping_rule)(enter_exit_framing | cross_surface), &rc);
    }

    bool can_use() const {
        if (!rc.hit)
            return false;

        auto *block = rc.block;
        auto bl = rc.bl;
        auto index = normal_to_surface_index(&rc);
        auto si = (surface_index)index;
        auto *other = ship->get_block(rc.p);

        // if we've started, only allow same plane
        if (state == paint_state::started) {
            if (other && other->type != block_empty && other->type != block_untouched) {
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

    void use(raycast_info *) override {
        if (!rc.hit)
            return;

        int index = normal_to_surface_index(&rc);
        auto si = (surface_index)index;

        if (can_use()) {
            switch (state) {
            case paint_state::started: {
                finish_paint(rc.bl);

                state = paint_state::idle;
                break;
            }
            case paint_state::idle: {
                start_block = rc.bl;
                start_index = si;
                select_type = rc.block->surfs[index];

                state = paint_state::started;
                break;
            }
            }
        }
    }

    void finish_paint(glm::ivec3 end) {
        auto cur = start_block;

        int i[2] = { 0, 0 };
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

    void alt_use(raycast_info *) override {
        switch (mode) {
        case replace_mode::all:
            mode = replace_mode::match;
            break;
        case replace_mode::match:
            mode = replace_mode::all;
            break;
        }
    }

    void long_use(raycast_info *) override {
        state = paint_state::idle;
    }

    void cycle_mode() override {
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

    void preview(raycast_info *, frame_data *frame) override {
        auto index = normal_to_surface_index(&rc);

        if (state == paint_state::started) {
            auto mesh = asset_man.get_surface_mesh(index, replace_type);
            auto material = asset_man.get_world_texture_index("red");

            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix = mat_position(glm::vec3(start_block));
            mat.ptr->material = material;
            mat.bind(1, frame);


            glUseProgram(overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            draw_mesh(mesh.hw);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
        }

        if (can_use()) {
            switch (state) {
            case paint_state::started: {
                auto cur = start_block;

                int i[2] = { 0, 0 };
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

                auto min = glm::min(start_block, rc.bl);
                auto max = glm::max(start_block, rc.bl);

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

                        auto mesh = asset_man.get_surface_mesh(index, replace_type);
                        auto material = asset_man.get_world_texture_index("red");

                        auto mat = frame->alloc_aligned<mesh_instance>(1);
                        mat.ptr->world_matrix = mat_position(glm::vec3(cur));
                        mat.ptr->material = material;
                        mat.bind(1, frame);

                        glUseProgram(overlay_shader);
                        glEnable(GL_POLYGON_OFFSET_FILL);
                        draw_mesh(mesh.hw);
                        glDisable(GL_POLYGON_OFFSET_FILL);
                        glUseProgram(simple_shader);
                    }
                }

                break;
            }
            case paint_state::idle: {
                auto mesh = asset_man.get_surface_mesh(index, replace_type);
                auto material = asset_man.get_world_texture_index("white");

                auto mat = frame->alloc_aligned<mesh_instance>(1);
                mat.ptr->world_matrix = mat_position(glm::vec3(rc.bl));
                mat.ptr->material = material;
                mat.bind(1, frame);

                glUseProgram(overlay_shader);
                glEnable(GL_POLYGON_OFFSET_FILL);
                draw_mesh(mesh.hw);
                glDisable(GL_POLYGON_OFFSET_FILL);
                glUseProgram(simple_shader);

                break;
            }
            }
        }
    }

    void get_description(char *str) override {
        auto m = mode == replace_mode::all ? "all" : "matching";
        sprintf(str, "Paint surface type %d replacing %s\n", (int)replace_type, m);
    }

    void select() override {
        state = paint_state::idle;
    }
};

tool *tool::create_paint_surface_tool() { return new paint_surface_tool; }