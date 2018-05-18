#include <epoxy/gl.h>
#include <glm/gtc/random.hpp>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "../player.h"
#include "../physics.h"
#include "tools.h"
#include "../entity_utils.h"
#include "../component/component_system_manager.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;
extern component_system_manager component_system_man;

struct remove_surface_tool : tool
{
    raycast_info_block rc;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, cross_surface, &rc);
    }

    bool can_use()
    {
        return rc.hit;
    }

    void use() override {
        if (!can_use())
            return;

        auto index = normal_to_surface_index(&rc);

        auto const &mesh = asset_man.surf_kinds.at(rc.block->surfs[index]);

        ship->set_surface(rc.bl, rc.p, (surface_index) index, surface_none);
        glm::ivec3 ch = ship->get_chunk_coord_containing(rc.bl);
        ship->get_chunk(ch)->prepare_phys(ch.x, ch.y, ch.z);

        /* remove any ents using the surface */
        remove_ents_from_surface(rc.p, index ^ 1);
        remove_ents_from_surface(rc.bl, index);

        auto e = spawn_floating_generic_entity(mat_block_face(glm::vec3(rc.p) - (glm::vec3) rc.n, index), mesh.legacy_mesh_name, mesh.legacy_popped_physics_mesh_name);
        auto &phys_man = component_system_man.managers.physics_component_man;
        auto phy = phys_man.get_instance_data(e);
        auto m = mat_rotate_mesh(rc.p, rc.n);
        auto v = m * glm::vec4(glm::diskRand(0.1f), 0.0f, 0.0f);
        (*phy.rigid)->applyImpulse(vec3_to_bt(rc.n) * 0.025f, vec3_to_bt(glm::vec3(v.x, v.y, v.z)));
    }

    void preview(frame_data *frame) override
    {
        if (!can_use())
            return;

        auto index = normal_to_surface_index(&rc);

        auto &mesh = asset_man.surf_kinds.at(rc.block->surfs[index]).visual_mesh;

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = mat_block_surface(glm::vec3(rc.bl), index ^ 1);
        mat.ptr->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
        mat.bind(1, frame);

        glUseProgram(overlay_shader);
        glEnable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(mesh->hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Remove surface");
    }
};

tool *tool::create_remove_surface_tool() { return new remove_surface_tool(); }
