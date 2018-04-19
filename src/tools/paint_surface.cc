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
extern player pl;
extern glm::mat4 get_fp_item_matrix();

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

    const mesh_data *get_fp_mesh() const {
        switch (replace_type) {
            case surface_wall:
                return &asset_man.get_mesh("fp_surface_wall");
            case surface_grate:
                return &asset_man.get_mesh("fp_surface_grate");
            case surface_glass:
                return &asset_man.get_mesh("fp_surface_glass");
            default:
                assert(false);
                return nullptr;
        }
    }

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, 
            (raycast_stopping_rule)(enter_exit_framing | cross_surface), &rc);
    }

    bool can_use() const {
        if (!rc.hit)
            return false;

        auto *block = rc.block;
        auto *other = ship->get_block(rc.p);

        // if we've started, only allow same plane
        if (state == paint_state::started) {
            if (other && other->type != block_empty && other->type != block_untouched) {
                return false;
            }

            if (idot(rc.n, rc.bl - start_block) != 0)
                return false;
        }

        return (block && block->type == block_frame);
    }

    void use(ship_space *ship) override {
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
        auto min = glm::min(start_block, end);
        auto max = glm::max(start_block, end);

        for (auto k = min.z; k <= max.z; k++) {
            for (auto j = min.y; j <= max.y; j++) {
                for (auto i = min.x; i <= max.x; i++) {
                    auto pos = glm::ivec3(i, j, k);
                    auto block = ship->get_block(pos);
                    if (!block || block->type != block_frame || (mode == replace_mode::match && block->surfs[start_index] != select_type)) {
                        continue;
                    }

                    ship->set_surface(pos, pos + surface_index_to_normal(start_index), start_index, replace_type);
                }
            }
        }
    }

    void alt_use() override {
        switch (mode) {
        case replace_mode::all:
            mode = replace_mode::match;
            break;
        case replace_mode::match:
            mode = replace_mode::all;
            break;
        }
    }

    void long_use() override {
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

    void preview(frame_data *frame) override {
        auto index = normal_to_surface_index(&rc);

        auto mesh = asset_man.surf_kinds[replace_type].visual_mesh;

        if (state == paint_state::started) {
            
            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix = mat_block_surface(start_block, index ^ 1);
            mat.ptr->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
            mat.bind(1, frame);

            glEnable(GL_BLEND);
            glUseProgram(overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            draw_mesh(mesh->hw);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
            glDisable(GL_BLEND);
        }

        auto fp_mesh = get_fp_mesh();

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = get_fp_item_matrix();
        mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat.bind(1, frame);
        draw_mesh(fp_mesh->hw);

        glUseProgram(overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_BLEND);

        if (can_use()) {
            switch (state) {
            case paint_state::started: {
                auto min = glm::min(start_block, rc.bl);
                auto max = glm::max(start_block, rc.bl);

                for (auto k = min.z; k <= max.z; k++) {
                    for (auto j = min.y; j <= max.y; j++) {
                        for (auto i = min.x; i <= max.x; i++) {
                            auto pos = glm::ivec3(i, j, k);
                            auto block = ship->get_block(pos);
                            if (!block || block->type != block_frame || (mode == replace_mode::match && block->surfs[start_index] != select_type)) {
                                continue;
                            }

                            auto mat = frame->alloc_aligned<mesh_instance>(1);
                            mat.ptr->world_matrix = mat_block_surface(pos, index ^ 1);
                            mat.ptr->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
                            mat.bind(1, frame);
                            draw_mesh(mesh->hw);
                        }
                    }
                }


                break;
            }
            case paint_state::idle: {
                auto mat = frame->alloc_aligned<mesh_instance>(1);
                mat.ptr->world_matrix = mat_block_surface(glm::vec3(rc.bl), index ^ 1);
                mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
                mat.bind(1, frame);
                draw_mesh(mesh->hw);
                break;
            }
            }
        }

        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override {
        auto m = mode == replace_mode::all ? "all" : "matching";
        sprintf(str, "Replace surfaces with %s (replacing %s)\n", asset_man.surf_kinds[(int)replace_type].name, m);
    }

    void select() override {
        state = paint_state::idle;
    }
};

tool *tool::create_paint_surface_tool() { return new paint_surface_tool; }