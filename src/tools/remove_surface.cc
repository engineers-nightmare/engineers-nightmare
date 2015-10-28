#include "../network.h"

#include <epoxy/gl.h>

#include "../network.h"

#include "../block.h"
#include "../common.h"
#include "../mesh.h"
#include "../ship_space.h"
#include "tools.h"
#include "../light_field.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;
extern ENetPeer *peer;

extern ship_space *ship;

extern hw_mesh *surfs_hw[6];

extern ENetPeer *peer;

extern void
remove_ents_from_surface(glm::ivec3 p, int face, physics *phy);

extern physics *phy;

struct remove_surface_tool : tool
{
    bool can_use(raycast_info *rc)
    {
        if (!rc->hit)
            return false;

        block *bl = rc->block;
        int index = normal_to_surface_index(rc);
        return bl && bl->surfs[index] & surface_phys;
    }

    void use(raycast_info *rc) override
    {
        if (!can_use(rc))
            return;

        int index = normal_to_surface_index(rc);

        ship->set_surface(rc->bl, rc->p, (surface_index)index, surface_none);
        set_block_surface(peer, rc->bl, rc->p, (surface_index)index, surface_none);

        /* remove any ents using the surface */
        remove_ents_from_surface(rc->p, index ^ 1, phy);
        remove_ents_from_surface(rc->bl, index, phy);

        mark_lightfield_update(rc->bl);
        mark_lightfield_update(rc->p);
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
