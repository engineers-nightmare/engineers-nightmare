#include <epoxy/gl.h>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;
extern player pl;

extern ship_space *ship;

extern asset_manager asset_man;
extern glm::mat4 get_fp_item_matrix();
extern glm::mat4 get_corner_matrix(block_type type, glm::ivec3 pos);

struct add_shaped_block_tool : tool
{
    raycast_info_block rc;
    block_type basic_type = block_corner_base;
    block_type type;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, enter_exit_framing, &rc);
        type = basic_type;

        if (rc.hit) {
            auto frac = rc.hitCoord - glm::floor(rc.hitCoord);
            if (rc.n.x < 0 || (frac.x > 0.5f && rc.n.x == 0)) {
                type = (block_type)(type | block_bit_xp);
            }
            if (rc.n.y < 0 || (frac.y > 0.5f && rc.n.y == 0)) {
                type = (block_type)(type | block_bit_yp);
            }
            if (rc.n.z < 0 || (frac.z > 0.5f && rc.n.z == 0)) {
                type = (block_type)(type | block_bit_zp);
            }
        }
    }

    bool can_use() {
        return rc.hit && !rc.inside;
    }

    void alt_use() override {
        if (basic_type == block_corner_base) {
            basic_type = block_invcorner_base;
        }
        else {
            basic_type = block_corner_base;
        }
    }

    void use() override
    {
        if (!can_use())
            return; /* n/a */

        /* ensure we can access this x,y,z */
        ship->ensure_block(rc.p);

        block *bl = ship->get_block(rc.p);

        bl->type = type;
        /* dirty the chunk */
        ship->get_chunk_containing(rc.p)->dirty();
    }

    void preview(frame_data *frame) override
    {
        auto mesh = asset_man.get_mesh(basic_type == block_corner_base ? "frame-corner" : "frame-invcorner");
        auto mesh2 = asset_man.get_mesh("fp_frame");    // TODO

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = get_fp_item_matrix();
        mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat.bind(1, frame);
        draw_mesh(mesh2.hw);

        if (!can_use())
            return; /* n/a */

        auto mat2 = frame->alloc_aligned<mesh_instance>(1);
        mat2.ptr->world_matrix = get_corner_matrix(type, rc.p);
        mat2.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat2.bind(1, frame);

        glUseProgram(overlay_shader);
        draw_mesh(mesh.hw);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Place Shaped Framing");
    }
};


tool *tool::create_add_shaped_block_tool() { return new add_shaped_block_tool(); }
