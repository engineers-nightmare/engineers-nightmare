#include <epoxy/gl.h>

#include "../ship_space.h"
#include "../shader_params.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern void
mark_lightfield_update(int x, int y, int z);

extern ship_space *ship;

extern glm::mat4
mat_position(float x, float y, float z);

extern hw_mesh *surfs_hw[6];

extern void
remove_ents_from_surface(int x, int y, int z, int face);


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
        ship->get_chunk_containing(rc->x, rc->y, rc->z)->render_chunk.valid = false;

        /* cause the other side to exist */
        block *other_side = ship->get_block(rc->p.x, rc->p.y, rc->p.z);

        if (!other_side) {
            /* expand: note: we shouldn't ever actually have to do this... */
        }
        else {
            other_side->surfs[index ^ 1] = surface_none;
            ship->get_chunk_containing(rc->p.x, rc->p.y, rc->p.z)->render_chunk.valid = false;
        }

        /* remove any ents using the surface */
        remove_ents_from_surface(rc->p.x, rc->p.y, rc->p.z, index ^ 1);
        remove_ents_from_surface(rc->x, rc->y, rc->z, index);

        mark_lightfield_update(rc->x, rc->y, rc->z);
        mark_lightfield_update(rc->p.x, rc->p.y, rc->p.z);

        ship->update_topology_for_remove_surface(rc->x, rc->y, rc->z, rc->p.x, rc->p.y, rc->p.z);
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {}

    void preview(raycast_info *rc) override
    {
        if (!can_use(rc))
            return;

        int index = normal_to_surface_index(rc);
        per_object->val.world_matrix = mat_position((float)rc->x, (float)rc->y, (float)rc->z);
        per_object->upload();

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
