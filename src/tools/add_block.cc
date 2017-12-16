#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;

struct add_block_tool : tool
{
    bool can_use(raycast_info *rc) {
        return rc->block.hit && !rc->block.inside;
    }

    void use(raycast_info *rc) override
    {
        if (!can_use(rc))
            return; /* n/a */

        /* ensure we can access this x,y,z */
        ship->ensure_block(rc->block.p);

        block *bl = ship->get_block(rc->block.p);

        /* can only build on the side of an existing frame */
        if (bl && rc->block.block->type == block_frame) {
            bl->type = block_frame;
            /* dirty the chunk */
            ship->get_chunk_containing(rc->block.p)->render_chunk.valid = false;
            ship->get_chunk_containing(rc->block.p)->phys_chunk.valid = false;
        }
    }

    void preview(raycast_info *rc, frame_data *frame) override
    {
        if (!can_use(rc))
            return; /* n/a */

        block *bl = ship->get_block(rc->block.p);

        /* can only build on the side of an existing frame */
        if ((!bl || bl->type == block_empty || bl->type == block_untouched) && rc->block.block->type == block_frame) {
            auto mesh = asset_man.get_mesh("frame");
            auto material = asset_man.get_world_texture_index("white");

            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix = mat_position(glm::vec3(rc->block.p));
            mat.ptr->material = material;
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
