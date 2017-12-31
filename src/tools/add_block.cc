#include <epoxy/gl.h>
#include <glm/ext.hpp>

#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "tools.h"
#include "../utils/debugdraw.h"
#include "../entity_utils.h"
#include "../component/component_system_manager.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;
extern player pl;

extern ship_space *ship;
extern component_system_manager component_system_man;

extern asset_manager asset_man;
extern glm::mat4 get_fp_item_matrix();

std::unordered_map<c_entity, std::vector<glm::vec3>> in_progress_frames;

struct add_block_tool : tool
{
    raycast_info_block rc;
    unsigned verts_index = UINT_MAX;

    void pre_use(player *pl) override {
        ship->raycast_block(pl->eye, pl->dir, MAX_REACH_DISTANCE, enter_exit_framing, &rc);
    }

    bool can_use() {
        return rc.hit && !rc.inside;
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

        auto pos = glm::vec3(get_fp_item_matrix()[3]);
        auto entity = spawn_unplaceable_entity("Builder Bot", mat_position(pos));
        auto bld = component_system_man.managers.builder_component_man.get_instance_data(entity);

        auto &mesh = asset_man.get_mesh("frame");

        auto src = mesh.sw;

        auto mat = mat_position(rc.p);

        auto num = src->num_vertices;
        auto step = 1u;
        if (num > 50) {
            step = num / 50;
            num = num / step;
        }

        auto place_verts = &in_progress_frames[entity];
        place_verts->reserve(num);
        for (auto i = 0u; i < src->num_vertices; i += step) {
            vertex v = src->verts[i];
            auto nv = mat * glm::vec4(v.x, v.y, v.z, 1);
            place_verts->emplace_back(nv);
        }

        *bld.build_index = 0;
        *bld.desired_pos = rc.p;
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
