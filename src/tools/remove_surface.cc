#include "../tools.h"

#include <epoxy/gl.h>
#include "../ship_space.h"
#include "../shader_params.h"
#include "../mesh.h"
#include "../block.h"


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
        block *other_side = ship->get_block(rc->px, rc->py, rc->pz);

        if (!other_side) {
            /* expand: note: we shouldn't ever actually have to do this... */
        }
        else {
            other_side->surfs[index ^ 1] = surface_none;
            ship->get_chunk_containing(rc->px, rc->py, rc->pz)->render_chunk.valid = false;
        }

        /* remove any ents using the surface */
        remove_ents_from_surface(rc->px, rc->py, rc->pz, index ^ 1);
        remove_ents_from_surface(rc->x, rc->y, rc->z, index);

        mark_lightfield_update(rc->x, rc->y, rc->z);
        mark_lightfield_update(rc->px, rc->py, rc->pz);

        ship->update_topology_for_remove_surface(rc->x, rc->y, rc->z, rc->px, rc->py, rc->pz);
    }

    void preview(raycast_info *rc) override
    {
        if (!can_use(rc))
            return;

        int index = normal_to_surface_index(rc);
        per_object->val.world_matrix = mat_position(rc->x, rc->y, rc->z);
        per_object->upload();

        glUseProgram(remove_overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-0.1, -0.1);
        draw_mesh(surfs_hw[index]);
        glPolygonOffset(0, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove surface");
    }
};

tool *tool::create_remove_surface_tool() { return new remove_surface_tool(); }
