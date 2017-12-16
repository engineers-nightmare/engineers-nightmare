#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;

struct remove_surface_tool : tool
{
    bool can_use(raycast_info *rc)
    {
        if (!rc->block.hit)
            return false;

        block *bl = rc->block.block;
        auto index = normal_to_surface_index(&rc->block);
        return bl && bl->surfs[index] & surface_phys;
    }

    void use(raycast_info *rc) override
    {
        if (!can_use(rc))
            return;

        auto index = normal_to_surface_index(&rc->block);

        ship->set_surface(rc->block.bl, rc->block.p, (surface_index)index, surface_none);

        /* remove any ents using the surface */
        remove_ents_from_surface(rc->block.p, index ^ 1);
        remove_ents_from_surface(rc->block.bl, index);
    }

    void preview(raycast_info *rc, frame_data *frame) override
    {
        if (!can_use(rc))
            return;

        block *bl = rc->block.block;
        auto index = normal_to_surface_index(&rc->block);

        auto mesh = asset_man.get_surface_mesh(index, bl->surfs[index]);
        auto material = asset_man.get_world_texture_index("red");

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_position(glm::vec3(rc->block.bl));
        mat.ptr->material = material;
        mat.bind(1, frame);

        glUseProgram(overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(mesh.hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(raycast_info *rc, char *str) override
    {
        strcpy(str, "Remove surface");
    }
};

tool *tool::create_remove_surface_tool() { return new remove_surface_tool(); }
