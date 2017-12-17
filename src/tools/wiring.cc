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

struct wire_pos { glm::ivec3 pos; unsigned face; };

static wire_pos from_rc(raycast_info_block const *rc) {
    return wire_pos{ rc->p, normal_to_surface_index(rc) ^ 1 };
}

struct wiring_tool : tool
{
    raycast_info_block rc;
    enum { idle, placing } state = idle;
    wire_pos start;

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
            ship->get_block(start.pos)->has_wire[start.face] = true;
            auto end = from_rc(&rc);
            ship->get_block(end.pos)->has_wire[end.face] = true;
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

        auto p = from_rc(&rc);
        ship->get_block(p.pos)->has_wire[p.face] = false;
    }

    void preview(frame_data *frame) override
    {
        if (state == placing) {
            auto mesh = asset_man.get_mesh("face_marker");
            auto material = asset_man.get_world_texture_index("red");

            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix = mat_block_face(glm::vec3(start.pos), start.face);
            mat.ptr->material = material;
            mat.bind(1, frame);

            glUseProgram(overlay_shader);
            draw_mesh(mesh.hw);
            glUseProgram(simple_shader);
        }

        if (!can_use())
            return; /* n/a */

        auto p = from_rc(&rc);

        auto mesh = asset_man.get_mesh("face_marker");
        auto material = asset_man.get_world_texture_index("white");

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_block_face(glm::vec3(p.pos), p.face);
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
