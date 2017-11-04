#include <epoxy/gl.h>

#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint add_overlay_shader;
extern GLuint remove_overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern std::array<std::string, face_count> surface_index_to_mesh;
extern std::unordered_map<std::string, mesh_data> meshes;

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

        /* remove any ents using the surface */
        remove_ents_from_surface(rc->p, index ^ 1);
        remove_ents_from_surface(rc->bl, index);
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

        auto mesh = meshes[surface_index_to_mesh[index]];

        glUseProgram(remove_overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(mesh.hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove surface");
    }
};

tool *tool::create_remove_surface_tool() { return new remove_surface_tool(); }
