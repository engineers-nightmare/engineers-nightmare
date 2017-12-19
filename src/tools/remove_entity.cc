#include <epoxy/gl.h>

#include "../glm/glm/gtc/random.hpp"

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "../physics.h"
#include "tools.h"
#include "../component/component_system_manager.h"


extern GLuint simple_shader;
extern GLuint highlight_shader;

extern ship_space *ship;

extern player pl;
extern physics *phy;

extern asset_manager asset_man;

extern void destroy_entity(c_entity e);

extern component_system_manager component_system_man;

struct remove_entity_tool : tool
{
    c_entity entity;
    raycast_info_world rc;

    void pre_use(player *pl) {
        phys_raycast_world(pl->eye, pl->eye + 2.f * pl->dir,
            phy->ghostObj.get(), phy->dynamicsWorld.get(), &rc);
    }

    bool can_use() {
        return rc.hit && c_entity::is_valid(rc.entity);
    }

    void use() override {
        if (!can_use())
            return;

        auto &phys = component_system_man.managers.physics_component_man;
        auto &pos = component_system_man.managers.relative_position_component_man;
        auto ph = phys.get_instance_data(entity);
        auto po = pos.get_instance_data(entity);
        convert_static_rb_to_dynamic(*ph.rigid, 1);
        (*ph.rigid)->applyCentralForce(vec3_to_bt(glm::sphericalRand(1.0f)));
    }

    void preview(frame_data *frame) override {
        auto &rend = component_system_man.managers.renderable_component_man;
        auto &pos_man = component_system_man.managers.relative_position_component_man;

        if (entity != rc.entity) {
            if (c_entity::is_valid(entity) && rend.exists(entity)) {
                *rend.get_instance_data(entity).draw = true;
            }

            pl.ui_dirty = true;    // TODO: untangle this.
            entity = rc.entity;

            if (c_entity::is_valid(entity) && rend.exists(entity)) {
                *rend.get_instance_data(entity).draw = false;
            }
        }

        if (c_entity::is_valid(entity) && rend.exists(entity)) {
            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix =  *pos_man.get_instance_data(entity).mat;
            mat.bind(1, frame);

            auto inst = rend.get_instance_data(entity);
            auto mesh = asset_man.get_mesh(*inst.mesh);
            glUseProgram(highlight_shader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            draw_mesh(mesh.hw);
            glDisable(GL_BLEND);
            glUseProgram(simple_shader);
        }
    }

    void get_description(char *str) override {
        auto &type_man = component_system_man.managers.type_component_man;
        if (c_entity::is_valid(entity) && type_man.exists(entity)) {
            auto type = type_man.get_instance_data(entity);
            sprintf(str, "Remove %s", *type.name);
        }
        else {
            strcpy(str, "Remove entity tool");
        }
    }

    void unselect() override {
        tool::unselect();
        if (c_entity::is_valid(entity)) {
            auto &rend = component_system_man.managers.renderable_component_man;
            *rend.get_instance_data(entity).draw = true;
        }
    }
};

tool *tool::create_remove_entity_tool() { return new remove_entity_tool(); }
