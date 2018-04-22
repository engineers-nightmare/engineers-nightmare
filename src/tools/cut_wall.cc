#include <epoxy/gl.h>
#include <glm/ext.hpp>
#include <array>
#include <limits>


#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "../player.h"
#include "tools.h"
#include "../utils/debugdraw.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;
extern player pl;

extern asset_manager asset_man;

struct cut_wall_tool : tool {
    raycast_info_block rc;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, enter_exit_framing, &rc);
    }

    bool can_use() {
        return rc.hit;
    }

    void use() override {
        if (!can_use()) return;

        auto door_center = glm::round(rc.hitCoord);
        auto index = normal_to_surface_index(&rc);

        if (~index & 1) {
            door_center -= glm::vec3(rc.n);
        }

        auto u = rc.n.x ? glm::vec3(0, 0, -1) : glm::vec3(-1, 0, 0);
        auto v = rc.n.y ? glm::vec3(0, 0, -1) : glm::vec3(0, -1, 0);

        glm::vec3 doors[] = {
            door_center,
            door_center + u,
            door_center + v,
            door_center + u + v,
        };

        // cut out door
        ship->cut_out_cuboid(glm::min(doors[0], doors[3]), glm::max(doors[0], doors[3]), surface_wall);

        ship->validate();
    }

    void preview(frame_data *frame) override {
        if (!rc.hit)
            return;

        if (can_use()) {
            auto door_center = glm::round(rc.hitCoord);
            auto index = normal_to_surface_index(&rc);

            if (~index & 1) {
                door_center -= glm::vec3(rc.n);
            }

            auto u = rc.n.x ? glm::vec3(0, 0, -1) : glm::vec3(-1, 0, 0);
            auto v = rc.n.y ? glm::vec3(0, 0, -1) : glm::vec3(0, -1, 0);

            glm::vec3 doors[] = {
                door_center,
                door_center + u,
                door_center + v,
                door_center + u + v,
            };

            glEnable(GL_BLEND);
            glUseProgram(overlay_shader);
            glEnable(GL_POLYGON_OFFSET_FILL);

            for (auto & door : doors) {
                auto mesh = asset_man.get_mesh("frame");

                auto mat = frame->alloc_aligned<mesh_instance>(1);
                mat.ptr->world_matrix = mat_position(glm::vec3(door));
                mat.ptr->color = glm::vec4(1.f, 0.f, 0.f, 1.f);
                mat.bind(1, frame);

                draw_mesh(mesh.hw);

            }

            glDisable(GL_POLYGON_OFFSET_FILL);
            glUseProgram(simple_shader);
            glDisable(GL_BLEND);
        }
    }

    void get_description(char *str) override {
        sprintf(str, "Cut wall");
    }
};


tool *tool::create_cut_wall_tool() { return new cut_wall_tool; }