#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../shader_params.h"
#include "../mesh.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern void
mark_lightfield_update(glm::ivec3 p);

extern ship_space *ship;

extern hw_mesh *scaffold_hw;

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
            remove_ents_from_surface(rc->bl, surface_zm);

            for (int face = 0; face < face_count; face++) {
                bl->surf_space[face] = 0;   /* we've just thrown away the block ent */
            }
        }

        /* block removal */
        bl->type = block_empty;

        /* strip any orphaned surfaces */
        for (int index = 0; index < 6; index++) {
            if (bl->surfs[index]) {

                auto s = surface_index_to_normal(index);

                auto r = rc->bl + s;
                block *other_side = ship->get_block(r);

                if (!other_side) {
                    /* expand: but this should always exist. */
                }
                else if (other_side->type != block_support) {
                    /* if the other side has no scaffold, then there is nothing left to support this
                     * surface pair -- remove it */
                    bl->surfs[index] = surface_none;
                    other_side->surfs[index ^ 1] = surface_none;
                    ship->get_chunk_containing(r)->render_chunk.valid = false;

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
            per_object->val.world_matrix = mat_position(rc->bl);
            per_object->upload();

            glUseProgram(remove_overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);
            draw_mesh(scaffold_hw);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
        }
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove Scaffolding");
    }
};

tool *tool::create_remove_block_tool() { return new remove_block_tool(); }
