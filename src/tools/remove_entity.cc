#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "../physics.h"
#include "tools.h"
#include "../component/component_system_manager.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

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

    void use(raycast_info *) override {
        if (!can_use())
            return;

        destroy_entity(rc.entity);
    }

    void preview(raycast_info *, frame_data *frame) override {
        if (entity != rc.entity) {
            pl.ui_dirty = true;    // TODO: untangle this.
            entity = rc.entity;
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
};

tool *tool::create_remove_entity_tool() { return new remove_entity_tool(); }
