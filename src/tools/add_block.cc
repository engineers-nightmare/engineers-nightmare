#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern hw_mesh *scaffold_hw;


struct add_block_tool : tool
{
    bool can_use(raycast_info *rc) {
        return rc->hit && !rc->inside;
    }

    void use(raycast_info *rc) override
    {
        if (!can_use(rc))
            return; /* n/a */

        /* ensure we can access this x,y,z */
        ship->ensure_block(rc->p);

        block *bl = ship->get_block(rc->p);

        /* can only build on the side of an existing scaffold */
        if (bl && rc->block->type == block_support) {
            bl->type = block_support;
            /* dirty the chunk */
            ship->get_chunk_containing(rc->p)->render_chunk.valid = false;
            ship->get_chunk_containing(rc->p)->phys_chunk.valid = false;
            mark_lightfield_update(rc->p);
        }
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {}

    void preview(raycast_info *rc, frame_data *frame) override
    {
        if (!can_use(rc))
            return; /* n/a */

        block *bl = ship->get_block(rc->p);

        /* can only build on the side of an existing scaffold */
        if ((!bl || bl->type == block_empty) && rc->block->type == block_support) {
            auto mat = frame->alloc_aligned<glm::mat4>(1);
            *mat.ptr = mat_position(rc->p);
            mat.bind(1, frame);

            glUseProgram(add_overlay_shader);
            draw_mesh(scaffold_hw);
            glUseProgram(simple_shader);
        }
    }

    void get_description(char *str) override
    {
        strcpy(str, "Place Scaffolding");
    }
};


tool *tool::create_add_block_tool() { return new add_block_tool(); }
