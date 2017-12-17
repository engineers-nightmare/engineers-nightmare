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

        block *bl = ship->get_block(rc.p);
        bl->has_wire[normal_to_surface_index(&rc) ^ 1] = true;
    }

    void alt_use() override
    {
        if (!can_use())
            return;

        block *bl = ship->get_block(rc.p);
        bl->has_wire[normal_to_surface_index(&rc) ^ 1] = false;
    }

    void preview(frame_data *frame) override
    {
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
};


tool *tool::create_wiring_tool() { return new wiring_tool(); }
