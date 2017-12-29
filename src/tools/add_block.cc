#include <epoxy/gl.h>
#include <glm/ext.hpp>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"
#include "../utils/debugdraw.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;
extern player pl;

extern ship_space *ship;

extern asset_manager asset_man;
extern glm::mat4 get_fp_item_matrix();

struct add_block_tool : tool
{
    raycast_info_block rc;
    std::vector<vertex> place_verts;
    unsigned verts_index = UINT_MAX;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, enter_exit_framing, &rc);
    }

    bool can_use() {
        return verts_index == UINT_MAX && rc.hit && !rc.inside;
    }

    void use() override
    {
        if (!can_use())
            return; /* n/a */

        /* ensure we can access this x,y,z */
        ship->ensure_block(rc.p);

        block *bl = ship->get_block(rc.p);

        bl->type = block_frame;
        /* dirty the chunk */
        ship->get_chunk_containing(rc.p)->dirty();

        auto &mesh = asset_man.get_mesh("frame");

        auto src = mesh.sw;

        auto mat = mat_position(rc.p);

        place_verts.resize(src->num_vertices);
        for (unsigned int i = 0; i < src->num_vertices; i++) {
            vertex v = src->verts[i];
            auto nv = mat * glm::vec4(v.x, v.y, v.z, 1);
            v.x = nv.x;
            v.y = nv.y;
            v.z = nv.z;
            place_verts[i] = v;
        }
        verts_index = 0;
    }

    void preview(frame_data *frame) override
    {
        auto mesh = asset_man.get_mesh("frame");
        auto mesh2 = asset_man.get_mesh("fp_frame");

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = get_fp_item_matrix();
        mat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat.bind(1, frame);
        draw_mesh(mesh2.hw);

        if (verts_index != UINT_MAX) {
            if (verts_index >= place_verts.size()) {
                verts_index = UINT_MAX;
            }
            else {
                auto plv = place_verts[verts_index];
                auto end = glm::vec3{plv.x, plv.y, plv.z};
                auto st = glm::vec3(get_fp_item_matrix()[3]);
                dd::line(glm::value_ptr(st), glm::value_ptr(end), dd::colors::CornflowerBlue, 60);
                verts_index += 25;
            }
        }

        if (!can_use())
            return; /* n/a */

        auto mat2 = frame->alloc_aligned<mesh_instance>(1);
        mat2.ptr->world_matrix = mat_position(glm::vec3(rc.p));
        mat2.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        mat2.bind(1, frame);

        glUseProgram(overlay_shader);
        draw_mesh(mesh.hw);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override
    {
        strcpy(str, "Place Framing");
    }
};


tool *tool::create_add_block_tool() { return new add_block_tool(); }
