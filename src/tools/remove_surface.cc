#include <epoxy/gl.h>

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
extern physics *phy;
extern player pl;

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

        auto const &mesh = asset_man.surf_kinds.at(rc.block->surfs[index]).legacy_mesh_name;

        ship->set_surface(rc.bl, rc.p, (surface_index) index, surface_none);

        /* remove any ents using the surface */
        remove_ents_from_surface(rc.p, index ^ 1);
        remove_ents_from_surface(rc.bl, index);

        // 0.9f is gross
        // it's there to ensure the face gets position this side of the old face enough that it comes out
        // instead of getting pushed into the frame
        auto e = spawn_floating_generic_entity(mat_block_face(glm::vec3(rc.p) - (glm::vec3) rc.n * 0.9f, index), mesh,
                                               mesh);

        /* remove tether if attached to this surface and attach to new entity */
        bool found = false;
        for (auto t = phy->tethers.begin(); t != phy->tethers.end();) {
            if ((*t).is_attached_to_surface(rc.bl, (surface_index) index, rc.p, (surface_index) (index ^ 1))) {
                (*t).detach(phy->dynamicsWorld.get());
                phy->tethers.erase(t);
                found = true;
                break;
            }
            else {
                t++;
            }
        }

//        if (!found) {
//            return;
//        }
//
//        auto &phys_man = component_system_man.managers.physics_component_man;
//        auto phys = phys_man.get_instance_data(e);
//        auto floater = *phys_man.get_instance_data(e).rigid;
//
//        btRigidBody *ignore = phy->rb_controller.get();
//        raycast_info_world erc;
//        phys_raycast_world(pl.eye, pl.eye + 6.f * rc.rayDir,
//                           ignore, phy->dynamicsWorld.get(), &erc);
//        ignore = floater;
//        raycast_info_world irc;
//        phys_raycast_world(erc.hitCoord + -pl.dir * 0.0001f, erc.hitCoord + 6.f * -pl.dir,
//                           ignore, phy->dynamicsWorld.get(), &irc);
//
//        phy->tethers.emplace_back();
//        auto *tether = &phy->tethers.back();
//        tether->attach_to_entity(phy->dynamicsWorld.get(), erc.hitCoord, floater, e);
//        tether->attach_to_rb(phy->dynamicsWorld.get(), irc.hitCoord, phy->rb_controller.get());
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
