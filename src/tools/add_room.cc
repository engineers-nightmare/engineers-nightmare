#include <epoxy/gl.h>
#include <glm/ext.hpp>
#include <array>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../block.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;

const static std::array<glm::ivec3, 3> room_sizes{glm::ivec3(3, 3, 3),
                                                  glm::ivec3(5, 5, 3),
                                                  glm::ivec3(7, 7, 3),
};

const static uint32_t valid_directions =
    (1<<surface_xp) | (1<<surface_xm) |
    (1<<surface_yp) | (1<<surface_ym);

struct add_room_tool : tool {
    unsigned room_size = 0;

    bool can_use(const raycast_info *rc) {
        auto *bl = ship->get_block(rc->bl);
        auto index = normal_to_surface_index(rc);

        auto u = surface_index_to_normal(surface_zp);

        glm::ivec3 down_block;
        if (!ship->find_next_block(rc->p, -u, 10, &down_block)) {
            return false;
        }

        return (valid_directions & (1<<index)) &&
            bl && bl->surfs[index] != surface_none;
    }

    void use(raycast_info *rc) override {
        if (!rc->hit)
            return;

        if (!can_use(rc)) return;

        auto size = room_sizes[room_size];
        auto f = -rc->n;
        auto u = surface_index_to_normal(surface_zp);
        glm::ivec3 ln = glm::ivec3(-f.y, f.x, f.z);

        auto next_block = rc->bl + f;
        auto front_left = next_block + (ln * (size.y / 2));
        auto back_right = next_block + (f * (size.x - 1)) - ln * (size.y / 2);

        // raycast down from hit block to get base
        glm::ivec3 down_block;
        if (!ship->find_next_block(rc->p, -u, 10, &down_block)) {
            return;
        }
        back_right.z = down_block.z + 1;

        // raycast up from hit block to get top of new room
        glm::ivec3 up_block;
        if (!ship->find_next_block(rc->p, u, 10, &up_block)) {
            up_block = down_block + u * size.z;
        }
        front_left.z = up_block.z - 1;

        auto mins = glm::min(front_left, back_right);
        auto maxs = glm::max(front_left, back_right);

        // cut out room
        ship->cut_out_cuboid(mins, maxs, surface_wall);

        // cut out door
        auto door_base = down_block + u + f;
        ship->cut_out_cuboid(door_base, door_base + u, surface_wall);

        ship->validate();
    }

    void alt_use(raycast_info *rc) override {}

    void long_use(raycast_info *rc) override {}

    void cycle_mode() override {
        room_size++;
        if (room_size >= room_sizes.size()) {
            room_size = 0;
        }
    }

    void preview(raycast_info *rc, frame_data *frame) override {
        if (!rc->hit)
            return;

        if (can_use(rc)) {
            auto f = -rc->n;
            auto u = surface_index_to_normal(surface_zp);

            // down to get base
            glm::ivec3 down_block;
            if (!ship->find_next_block(rc->p, -u, 10, &down_block)) {
                return;
            }

            std::array<glm::ivec3, 2> doors;
            doors[0] = down_block + u + f;
            doors[1] = doors[0] + u;


            for (auto & door : doors) {
                auto mesh = asset_man.get_mesh("frame");
                auto material = asset_man.get_world_texture_index("red");

                auto mat = frame->alloc_aligned<mesh_instance>(1);
                mat.ptr->world_matrix = mat_position(glm::vec3(door));
                mat.ptr->material = material;
                mat.bind(1, frame);

                glUseProgram(overlay_shader);
                glEnable(GL_POLYGON_OFFSET_FILL);
                draw_mesh(mesh.hw);
                glDisable(GL_POLYGON_OFFSET_FILL);
                glUseProgram(simple_shader);
            }
        }
    }

    void get_description(char *str) override {
        sprintf(str, "Add room: %s\n", glm::to_string(room_sizes[room_size]).c_str());
    }
};


tool *tool::create_add_room_tool() { return new add_room_tool; }