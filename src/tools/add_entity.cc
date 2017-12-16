#include "../tinydir.h"

#include <epoxy/gl.h>
#include <glm/gtx/transform.hpp>

#include "../component/component_system_manager.h"
#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"

#include <libconfig.h>
#include "../libconfig_shim.h"
#include "../entity_utils.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;
extern component_system_manager component_system_man;

extern std::vector<std::string> entity_names;
extern std::unordered_map<std::string, entity_data> entity_stubs;

extern frame_info frame_info;

struct add_entity_tool : tool {
    const unsigned rotate_tick_rate = 5; // hz
    double last_rotate_time = 0;
    unsigned cur_rotate = 0;
    unsigned entity_name_index = 0;
    raycast_info_block rc;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, cross_surface, &rc);
    }

    // todo: we need to check against entities already placed in the world for interpenetration
    // todo: we need to check to ensure that this placement won't embed us in a block/is on a full base
    bool can_use() {
        if (!rc.hit) {
            return false;
        }

        block *bl = rc.block;

        if (!bl) {
            return false;
        }

        return true;
    }

    unsigned get_rotate() const {
        auto place = entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>();

        switch (place->rot) {
        case rotation::axis_aligned: {
            return 90;
        }
        case rotation::rot_45: {
            return 45;
        }
        case rotation::rot_15: {
            return 15;
        }
        case rotation::no_rotation: {
            return 0;
        }
        default:
            assert(false);
            return 0;
        }
    }

    void use() override {
        if (!can_use()) {
            return;
        }

        auto index = normal_to_surface_index(&rc);

        chunk *ch = ship->get_chunk_containing(rc.p);
        /* the chunk we're placing into is guaranteed to exist, because there's
        * a surface facing into it */
        assert(ch);

        glm::mat4 mat = get_place_matrix(index);

        auto name = entity_names[entity_name_index];
        auto e = spawn_entity(name, rc.p, index ^ 1, mat);
        ch->entities.push_back(e);
    }

    // press to rotate once, hold to rotate continuously
    void alt_use() override {
        auto rotate = get_rotate();

        cur_rotate += rotate;
        cur_rotate %= 360;
    }

    // hold to rotate continuously, press to rotate once
    void long_alt_use() override {
        auto rotate = get_rotate();

        if (frame_info.elapsed >= last_rotate_time + 1.0 / rotate_tick_rate) {
            cur_rotate += rotate;
            cur_rotate %= 360;
            last_rotate_time = frame_info.elapsed;
        }
    }

    void cycle_mode() override {
        do {
            entity_name_index++;
            if (entity_name_index >= entity_names.size()) {
                entity_name_index = 0;
            }
        } while (!entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>());

        auto rotate = get_rotate();
        if (rotate == 0) {
            cur_rotate = 0;
        }
        else {
            cur_rotate += rotate / 2;
            cur_rotate = cur_rotate - (cur_rotate % rotate);
        }
    }

    void preview(frame_data *frame) override {
        if (!can_use())
            return;

        auto index = normal_to_surface_index(&rc);
        auto render = entity_stubs[entity_names[entity_name_index]].get_component<renderable_component_stub>();

        glm::mat4 m = get_place_matrix(index);

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = m;
        mat.bind(1, frame);

        glUseProgram(simple_shader);
        auto mesh = asset_man.get_mesh(render->mesh);
        draw_mesh(mesh.hw);

        // draw the placement overlay
        auto surf_mesh = asset_man.get_mesh("placement_overlay");
        auto material = asset_man.get_world_texture_index("placement_overlay");

        auto mat2 = frame->alloc_aligned<mesh_instance>(1);
        mat2.ptr->world_matrix = m;
        mat2.ptr->material = material;
        mat2.bind(1, frame);

        glUseProgram(overlay_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(surf_mesh.hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void get_description(char *str) override {
        auto name = entity_names[entity_name_index];

        sprintf(str, "Place %s", name.c_str());
    }

    glm::mat4 get_place_matrix(unsigned int index) const {
        glm::mat4 m;
        auto rot_axis = glm::vec3{surface_index_to_normal(surface_zp)};

        auto place = entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>();

        float step = 1;
        switch (place->place) {
            case placement::eighth_block_snapped: {
                step *= 2;
                // falls through
            }
            case placement::quarter_block_snapped: {
                step *= 2;
                // falls through
            }
            case placement::half_block_snapped: {
                step *= 2;
                auto pos = rc.hitCoord;
                m = mat_rotate_mesh(pos, rc.n);
                m[3].x = std::round(m[3].x * step) / step;
                m[3].y = std::round(m[3].y * step) / step;
                m[3].z = std::round(m[3].z * step) / step;
                break;
            }
            case placement::full_block_snapped: {
                m = mat_block_face(rc.p, index ^ 1);
                break;
            }
            default:
                assert(false);
        }
        m = rotate(m, -glm::radians((float) cur_rotate), rot_axis);
        return m;
    }
};

tool *tool::create_add_entity_tool() { return new add_entity_tool; }
