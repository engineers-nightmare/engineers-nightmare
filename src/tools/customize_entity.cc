#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"
#include "../component/component_system_manager.h"

extern GLuint simple_shader;
extern GLuint highlight_shader;

extern ship_space *ship;

extern physics *phy;

extern asset_manager asset_man;
extern player pl;
extern glm::mat4 get_fp_item_matrix();

extern void destroy_entity(c_entity e);

extern component_system_manager component_system_man;

struct customize_entity_tool : tool
{
    c_entity entity;
    raycast_info_world rc;

    void pre_use(player *pl) {
        phys_raycast_world(pl->eye, pl->eye + 2.f * pl->dir,
                           phy->rb_controller.get(), phy->dynamicsWorld.get(), &rc);
    }

    bool can_use() {
        if (!rc.hit || !c_entity::is_valid(entity)) {
            return false;
        }

        auto &sam = component_system_man.managers.surface_attachment_component_man;
        return sam.exists(entity) && *sam.get_instance_data(entity).attached;
    }

    void use() override {
        if (!can_use())
            return;

        auto &rend = component_system_man.managers.renderable_component_man;
        *rend.get_instance_data(entity).draw = true;
    }

    void preview(frame_data *frame) override {
        auto &rend = component_system_man.managers.renderable_component_man;
        auto &pos_man = component_system_man.managers.position_component_man;

        auto fp_mesh = &asset_man.get_mesh("fp_customize_tool");


        glm::vec3 fp_item_offset{ 0.115f, 0.2f, -0.12f };
        float fp_item_scale{ 0.175f };
        glm::quat fp_item_rot{ -5.f, 5.f, 5.f, 5.f };

        auto m = glm::mat4_cast(glm::normalize(pl.rot));
        auto right = glm::vec3(m[0]);
        auto up = glm::vec3(m[1]);

        auto fpmat = frame->alloc_aligned<mesh_instance>(1);
        fpmat.ptr->world_matrix = glm::scale(mat_position(pl.eye + pl.dir * fp_item_offset.y + right * fp_item_offset.x + up * fp_item_offset.z), glm::vec3(fp_item_scale)) * m * glm::mat4_cast(glm::normalize(fp_item_rot));
        fpmat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        fpmat.bind(1, frame);
        draw_mesh(fp_mesh->hw);

        if (entity != rc.entity) {
            if (c_entity::is_valid(entity) && rend.exists(entity)) {
                *rend.get_instance_data(entity).draw = true;
            }

            pl.ui_dirty = true;    // TODO: untangle this.
            entity = rc.entity;

            if (can_use() && c_entity::is_valid(entity) && rend.exists(entity)) {
                *rend.get_instance_data(entity).draw = false;
            }
        }

        if (can_use() && c_entity::is_valid(entity) && rend.exists(entity)) {
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
        if (can_use()) {
            auto type = type_man.get_instance_data(entity);
            sprintf(str, "Customize %s", *type.name);
        }
        else {
            strcpy(str, "Customize entity tool");
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

tool *tool::create_customize_entity_tool() { return new customize_entity_tool(); }
