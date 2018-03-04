#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;
extern player pl;

extern ship_space *ship;

extern asset_manager asset_man;
extern glm::mat4 get_fp_item_matrix();
extern glm::mat4 get_corner_matrix(block_type type, glm::ivec3 pos);

struct add_shaped_block_tool : tool
{
    raycast_info_block rc;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, enter_exit_framing, &rc);
    }

    bool can_use() {
        return rc.hit && !rc.inside;
    }

    void use() override
    {
        if (!can_use())
            return; /* n/a */

        /* ensure we can access this x,y,z */
        ship->ensure_block(rc.p);

        block *bl = ship->get_block(rc.p);

        bl->type = block_frame;
        /* dirty the chunk */
        ship->get_chunk_containing(rc.p)->dirty();
    }

    void preview(frame_data *frame) override
    {
        auto mesh = asset_man.get_mesh("frame");
        auto mesh2 = asset_man.get_mesh("fp_frame");

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = get_fp_item_matrix();
        mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat.bind(1, frame);
        draw_mesh(mesh2.hw);

        if (!can_use())
            return; /* n/a */

        auto mat2 = frame->alloc_aligned<mesh_instance>(1);
        mat2.ptr->world_matrix = mat_position(glm::vec3(rc.p));
        mat2.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat2.bind(1, frame);

        glUseProgram(overlay_shader);
        draw_mesh(mesh.hw);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Place Shaped Framing");
    }
};


tool *tool::create_add_shaped_block_tool() { return new add_shaped_block_tool(); }
