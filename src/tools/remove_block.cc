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

extern mesh_data const * mesh_for_block_type(block_type t);
extern glm::mat4 get_corner_matrix(block_type type, glm::ivec3 pos);

struct remove_block_tool : tool
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
            return;

        ship->remove_block(rc.bl);
    }

    void preview(frame_data *frame) override
    {
        if (!can_use())
            return;

        block *bl = rc.block;
        if (bl->type != block_empty && bl->type != block_untouched) {
            auto mesh = mesh_for_block_type(bl->type);

            auto mat = frame->alloc_aligned<mesh_instance>(1);
            if (bl->type == block_frame) {
                mat.ptr->world_matrix = mat_position(glm::vec3(rc.bl));
            }
            else {
                mat.ptr->world_matrix = get_corner_matrix(bl->type, rc.bl);
            }
            mat.ptr->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
            mat.bind(1, frame);

            glUseProgram(overlay_shader);
            glEnable(GL_BLEND);
            glEnable(GL_POLYGON_OFFSET_FILL);
            draw_mesh(mesh->hw);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_BLEND);
            glUseProgram(simple_shader);
        }
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove Framing");
    }
};

tool *tool::create_remove_block_tool() { return new remove_block_tool(); }
