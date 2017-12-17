#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;

struct wiring_tool : tool
{
    raycast_info_block rc;
    enum { idle, placing } state = idle;
    glm::ivec3 start_pos;
    int start_face;

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
            start_pos = rc.p;
            start_face = normal_to_surface_index(&rc) ^ 1;
            state = placing;
            break;

        case placing: {
            ship->get_block(start_pos)->has_wire[start_face] = true;
            auto end_face = normal_to_surface_index(&rc) ^ 1;
            ship->get_block(rc.p)->has_wire[end_face] = true;
            state = idle;
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

        ship->get_block(rc.p)->has_wire[normal_to_surface_index(&rc) ^ 1] = false;
    }

    void preview(frame_data *frame) override
    {
        if (state == placing) {
            block *bl = ship->get_block(rc.p);

            auto mesh = asset_man.get_mesh("face_marker");
            auto material = asset_man.get_world_texture_index("red");

            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix = mat_block_face(glm::vec3(start_pos), start_face);
            mat.ptr->material = material;
            mat.bind(1, frame);

            glUseProgram(overlay_shader);
            draw_mesh(mesh.hw);
            glUseProgram(simple_shader);
        }

        if (!can_use())
            return; /* n/a */

        block *bl = ship->get_block(rc.p);

        auto mesh = asset_man.get_mesh("face_marker");
        auto material = asset_man.get_world_texture_index("white");

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_block_face(glm::vec3(rc.p), normal_to_surface_index(&rc) ^ 1);
        mat.ptr->material = material;
        mat.bind(1, frame);

        glUseProgram(overlay_shader);
        draw_mesh(mesh.hw);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Wiring");
    }

    void select() override { state = idle; }
};


tool *tool::create_wiring_tool() { return new wiring_tool(); }
