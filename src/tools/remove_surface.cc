#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern void
mark_lightfield_update(glm::ivec3 p);

extern ship_space *ship;

extern hw_mesh *surfs_hw[6];

extern void
remove_ents_from_surface(glm::ivec3 p, int face);


struct remove_surface_tool : tool
{
    bool can_use(raycast_info *rc)
    {
        if (!rc->hit)
            return false;

        block *bl = rc->block;
        int index = normal_to_surface_index(rc);
        return bl && bl->surfs[index] != surface_none;
    }

    void use(raycast_info *rc) override
    {
        if (!can_use(rc))
            return;

        block *bl = rc->block;

        int index = normal_to_surface_index(rc);

        bl->surfs[index] = surface_none;
        ship->get_chunk_containing(rc->bl)->render_chunk.valid = false;

        /* cause the other side to exist */
        block *other_side = ship->get_block(rc->p);

        if (!other_side) {
            /* expand: note: we shouldn't ever actually have to do this... */
        }
        else {
            other_side->surfs[index ^ 1] = surface_none;
            ship->get_chunk_containing(rc->p)->render_chunk.valid = false;
        }

        /* remove any ents using the surface */
        remove_ents_from_surface(rc->p, index ^ 1);
        remove_ents_from_surface(rc->bl, index);

        mark_lightfield_update(rc->bl);
        mark_lightfield_update(rc->p);

        ship->update_topology_for_remove_surface(rc->bl, rc->p);
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {}

    void preview(raycast_info *rc, frame_data *frame) override
    {
        if (!can_use(rc))
            return;

        int index = normal_to_surface_index(rc);
        auto mat = frame->alloc_aligned<glm::mat4>(1);
        *mat.ptr = mat_position(rc->bl);
        mat.bind(1, frame);

        glUseProgram(remove_overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(surfs_hw[index]);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove surface");
    }
};

tool *tool::create_remove_surface_tool() { return new remove_surface_tool(); }
