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

struct add_block_tool : tool
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

        /* can only build on the side of an existing frame */
        if (bl && rc.block->type == block_frame) {
            bl->type = block_frame;
            /* dirty the chunk */
            ship->get_chunk_containing(rc.p)->render_chunk.valid = false;
            ship->get_chunk_containing(rc.p)->phys_chunk.valid = false;
        }
    }

    void preview(frame_data *frame) override
    {
        auto mesh = asset_man.get_mesh("frame");
        auto mesh2 = asset_man.get_mesh("frame2");

        auto m = glm::mat4_cast(glm::normalize(pl.rot));
        auto right = glm::vec3(m[0]);
        auto up = glm::vec3(m[1]);

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = glm::scale(mat_position(pl.eye + pl.dir * 0.2f + right * 0.2f - up * 0.1f), glm::vec3(0.2f)) * m * glm::mat4_cast(glm::normalize(glm::quat(1.f,2.f,3.f,2.f)));
        mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat.bind(1, frame);
        draw_mesh(mesh2.hw);

        if (!can_use())
            return; /* n/a */

        block *bl = ship->get_block(rc.p);

        /* can only build on the side of an existing frame */
        if ((!bl || bl->type == block_empty || bl->type == block_untouched) && rc.block->type == block_frame) {
            auto mesh = asset_man.get_mesh("frame");

            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix = mat_position(glm::vec3(rc.p));
            mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
            mat.bind(1, frame);

            glUseProgram(overlay_shader);
            draw_mesh(mesh.hw);
            glUseProgram(simple_shader);
        }
    }

    void get_description(char *str) override
    {
        strcpy(str, "Place Framing");
    }
};


tool *tool::create_add_block_tool() { return new add_block_tool(); }
