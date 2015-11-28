#include "../network.h"

#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "tools.h"
#include "../light_field.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern hw_mesh *frame_hw;

extern void
remove_ents_from_surface(glm::ivec3 p, int face, physics *phy);

extern physics *phy;
extern ENetPeer *peer;

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
            /* TODO: should this even allow entity removal?
             * This may be nothing more than historical accident.
             * TODO: server needs to know about entity removal
             */
            remove_ents_from_surface(rc->bl, surface_zm, phy);
            mark_lightfield_update(rc->bl);
            return;
        }

        /* block removal */
        /* send block removal to server */
        set_block_type(peer, rc->bl, block_empty);

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
                    /* if the other side has no frame, then there is nothing
                     * left to support this surface pair -- remove it */

                    auto si = (surface_index)index;
                    auto oi = (surface_index)(index ^ 1);

                    /* send surface update to server
                     * todo: batch these together
                     */
                    set_block_surface(peer, rc->bl, r, si, surface_none);
                    set_block_surface(peer, r, rc->bl, oi, surface_none);

                    /* pop any dependent ents */
                    remove_ents_from_surface(rc->bl, index, phy);
                    remove_ents_from_surface(r, index ^ 1, phy);
                }
            }
        }
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
