#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern hw_mesh *frame_hw;

extern void
remove_ents_from_surface(glm::ivec3 p, int face);


struct remove_block_tool : tool
{
    bool can_use(raycast_info *rc) {
        return rc->hit && !rc->inside;
    }

    void use(raycast_info *rc) override
    {
        if (!can_use(rc))
            return;

        block *bl = rc->block;

        /* if there was a block entity here, find and remove it. block
         * ents are "attached" to the zm surface */
        if (bl->type == block_entity) {
            /* TODO: should this even allow entity removal? This may be nothing more than
             * historical accident.
             */
            remove_ents_from_surface(rc->bl, surface_zm);
            mark_lightfield_update(rc->bl);
            return;
        }

        /* block removal */
        bl->type = block_empty;

        /* strip any orphaned surfaces */
        for (int index = 0; index < 6; index++) {
            if (bl->surfs[index] & surface_phys) {

                auto s = surface_index_to_normal(index);

                auto r = rc->bl + s;
                block *other_side = ship->get_block(r);

                if (!other_side) {
                    /* expand: but this should always exist. */
                }
                else if (other_side->type != block_frame) {
                    /* if the other side has no frame, then there is nothing left to support this
                     * surface pair -- remove it */
                    bl->surfs[index] = surface_none;
                    other_side->surfs[index ^ 1] = surface_none;
                    ship->get_chunk_containing(r)->render_chunk.valid = false;
                    ship->get_chunk_containing(r)->phys_chunk.valid = false;

                    /* pop any dependent ents */
                    remove_ents_from_surface(rc->bl, index);
                    remove_ents_from_surface(r, index ^ 1);

                    mark_lightfield_update(r);

                    ship->update_topology_for_remove_surface(rc->bl, r);
                }
            }
        }

        /* dirty the chunk */
        ship->get_chunk_containing(rc->bl)->render_chunk.valid = false;
        ship->get_chunk_containing(rc->bl)->phys_chunk.valid = false;
        mark_lightfield_update(rc->bl);
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {}

    void preview(raycast_info *rc, frame_data *frame) override
    {
        if (!can_use(rc))
            return;

        block *bl = rc->block;
        if (bl->type != block_empty) {
            auto mat = frame->alloc_aligned<glm::mat4>(1);
            *mat.ptr = mat_position(rc->bl);
            mat.bind(1, frame);

            glUseProgram(remove_overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            draw_mesh(frame_hw);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
        }
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove Framing");
    }
};

tool *tool::create_remove_block_tool() { return new remove_block_tool(); }
