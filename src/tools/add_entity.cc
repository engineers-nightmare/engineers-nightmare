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
extern player pl;

extern asset_manager asset_man;
extern component_system_manager component_system_man;

extern std::vector<std::string> entity_names;
extern std::unordered_map<std::string, entity_data> entity_stubs;

struct add_entity_tool : tool {
    unsigned entity_name_index = 0;
    raycast_info_block rc;

    void select() override {
        if (!entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>())
            cycle_mode();   // make sure we've chosen a valid entity.
    }

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

    int get_rotate() const {
        auto place = entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>();
        return place->rot;
    }

    void use() override {
        if (!can_use()) {
            return;
        }

        auto index = normal_to_surface_index(&rc);

        glm::mat4 mat = get_place_matrix(index);

        auto name = entity_names[entity_name_index];
        auto e = spawn_entity(name, mat);
        attach_entity_to_surface(e, rc.p, index ^ 1);
    }

    void cycle_mode() override {
        do {
            entity_name_index++;
            if (entity_name_index >= entity_names.size()) {
                entity_name_index = 0;
            }
        } while (!entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>());
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
    }

    void get_description(char *str) override {
        auto name = entity_names[entity_name_index];

        sprintf(str, "Place %s", name.c_str());
    }

    float get_best_rotation(glm::vec3 axis) const {
        auto rotate = get_rotate();
        auto bestdot = std::numeric_limits<float>::lowest();
        auto best = 0;
        auto up = glm::transpose(glm::mat3_cast(pl.rot))[1];
        if (rotate) {
            for (int angle = 0; angle < 360; angle += rotate) {
                auto candidate = glm::rotate(glm::mat4(1), -glm::radians((float)angle), axis);
                auto vec = glm::vec3(glm::transpose(candidate)[1]);
                auto dot = glm::dot(vec, up);

                if (dot > bestdot) {
                    best = angle;
                    bestdot = dot;
                }
            }
        }
        return (float) best;
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
        m = glm::rotate(m, -glm::radians((float)get_best_rotation(rot_axis)), rot_axis);
        return m;
    }
};

tool *tool::create_add_entity_tool() { return new add_entity_tool; }
