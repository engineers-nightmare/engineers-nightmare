#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "tools.h"
#include "../component/component_system_manager.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern player pl;

extern asset_manager asset_man;

extern void destroy_entity(c_entity e);

extern component_system_manager component_system_man;

struct remove_entity_tool : tool
{
    bool dirty = false;

    bool can_use(raycast_info *rc) {
        return rc->world.hit && c_entity::is_valid(rc->world.entity);
    }

    void use(raycast_info *rc) override {
        if (!can_use(rc))
            return;

        destroy_entity(rc->world.entity);
    }

    void preview(raycast_info *rc, frame_data *frame) override {
        auto use = can_use(rc);
        if (use != dirty) {
            pl.ui_dirty = true;
            use = !use;
        }
    }

    void get_description(raycast_info *rc, char *str) override {
        if (rc->world.hit && c_entity::is_valid(rc->world.entity)) {
            auto &type_man = component_system_man.managers.type_component_man;
            auto type = type_man.get_instance_data(rc->world.entity);
            sprintf(str, "Remove %s", *type.name);
        }
        else {
            strcpy(str, "Remove entity tool");
        }
    }
};

tool *tool::create_remove_entity_tool() { return new remove_entity_tool(); }
