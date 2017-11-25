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
extern player pl;

extern std::vector<std::string> entity_names;
extern std::unordered_map<std::string, entity_data> entity_stubs;

extern frame_info frame_info;

struct add_entity_tool : tool {
    enum class RotateMode {
        AxisAligned,
        Stepped45,
        Stepped15,
    } rotate_mode{RotateMode::AxisAligned};

    enum class PlaceMode {
        BlockSnapped,
        HalfBlockSnapped,
        QuarterBlockSnapped,
        EighthBlockSnapped,
        FreeForm,
    } place_mode{PlaceMode::BlockSnapped};

    const unsigned rotate_tick_rate = 5; // hz
    double last_rotate_time = 0;
    unsigned cur_rotate = 0;

    unsigned entity_name_index = 0;

    // todo: we need to check against entities already placed in the world for interpenetration
    // todo: we need to check to ensure that this placement won't embed us in a block/is on a full base
    bool can_use(raycast_info *rc) {
        if (!rc->hit) {
            return false;
        }

        block *bl = rc->block;

        if (!bl) {
            return false;
        }

        int index = normal_to_surface_index(rc);

        if (~bl->surfs[index] & surface_phys) {
            return false;
        }

        return true;
    }

    void use(raycast_info *rc) override {
        if (!can_use(rc)) {
            return;
        }

        auto index = normal_to_surface_index(rc);

        chunk *ch = ship->get_chunk_containing(rc->p);
        /* the chunk we're placing into is guaranteed to exist, because there's
        * a surface facing into it */
        assert(ch);

        glm::mat4 mat = get_place_matrix(rc, index);

        auto name = entity_names[entity_name_index];
        auto e = spawn_entity(name, rc->p, index ^ 1, mat);
        ch->entities.push_back(e);

        place_entity_attaches(rc, index, e);
    }

    void alt_use(raycast_info *rc) override {
        entity_name_index++;
        if (entity_name_index>= entity_names.size()) {
            entity_name_index = 0;
        }
    }

    void long_use(raycast_info *rc) override {
        switch (rotate_mode) {
            case RotateMode::AxisAligned: {
                rotate_mode = RotateMode::Stepped45;
                break;
            }
            case RotateMode::Stepped45: {
                rotate_mode = RotateMode::Stepped15;
                break;
            }
            case RotateMode::Stepped15: {
                rotate_mode = RotateMode::AxisAligned;
                cur_rotate = 0;
                break;
            }
        }
    }

    void long_alt_use(raycast_info *rc) override {
        unsigned rotate = 90;

        switch (rotate_mode) {
            case RotateMode::AxisAligned: {
                rotate = 90;
                break;
            }
            case RotateMode::Stepped45: {
                rotate = 45;
                break;
            }
            case RotateMode::Stepped15: {
                rotate = 15;
                break;
            }
        }

        if (frame_info.elapsed >= last_rotate_time + 1.0 / rotate_tick_rate) {
            cur_rotate += rotate;
            cur_rotate %= 360;
            last_rotate_time = frame_info.elapsed;
            printf("%d\n", cur_rotate);
        }
    }

    void cycle_mode() override {
        switch (place_mode) {
            case PlaceMode::BlockSnapped: {
                place_mode = PlaceMode::HalfBlockSnapped;
                break;
            }
            case PlaceMode::HalfBlockSnapped: {
                place_mode = PlaceMode::QuarterBlockSnapped;
                break;
            }
            case PlaceMode::QuarterBlockSnapped: {
                place_mode = PlaceMode::EighthBlockSnapped;
                break;
            }
            case PlaceMode::EighthBlockSnapped: {
                place_mode = PlaceMode::FreeForm;
                break;
            }
            case PlaceMode::FreeForm: {
                place_mode = PlaceMode::BlockSnapped;
                break;
            }
        }
    }

    void preview(raycast_info *rc, frame_data *frame) override {
        if (!can_use(rc))
            return;

        auto index = normal_to_surface_index(rc);
        auto render = entity_stubs[entity_names[entity_name_index]].get_component<renderable_component_stub>();

        glm::mat4 m = get_place_matrix(rc, index);

        auto mat = frame->alloc_aligned<mesh_instance>(1);
        mat.ptr->world_matrix = m;
        mat.ptr->material = asset_man.get_world_texture_index(render->material);
        mat.bind(1, frame);

        auto mesh = asset_man.get_mesh(render->mesh);
        draw_mesh(mesh.hw);

        // draw the placement overlay
        auto surf_mesh = asset_man.get_mesh("placement_overlay");
        auto material = asset_man.get_world_texture_index("placement_overlay");

        auto mat2 = frame->alloc_aligned<mesh_instance>(1);
        mat2.ptr->world_matrix = m;
        mat2.ptr->material = material;
        mat2.bind(1, frame);

        glUseProgram(simple_shader);
        glEnable(GL_POLYGON_OFFSET_FILL);
        draw_mesh(surf_mesh.hw);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(simple_shader);
    }

    void get_description(char *str) override {
        auto name = entity_names[entity_name_index];
        const char *rotate = nullptr;
        switch (rotate_mode) {
            case RotateMode::AxisAligned: {
                rotate = "Axis Aligned";
                break;
            }
            case RotateMode::Stepped45: {
                rotate = "Stepped @ 45";
                break;
            }
            case RotateMode::Stepped15: {
                rotate = "Stepped @ 15";
                break;
            }
        }

        const char *place = nullptr;
        switch (place_mode) {
            case PlaceMode::BlockSnapped: {
                place = "Block Snapped";
                break;
            }
            case PlaceMode::HalfBlockSnapped: {
                place = "Half Block Snapped";
                break;
            }
            case PlaceMode::QuarterBlockSnapped: {
                place = "Quarter Block Snapped";
                break;
            }
            case PlaceMode::EighthBlockSnapped: {
                place = "Eighth Block Snapped";
                break;
            }
            case PlaceMode::FreeForm: {
                place = "Free Form";
                break;
            }
        }
        sprintf(str, "Place %s \nRotating %s \nPlacing %s", name.c_str(), rotate, place);
    }

    glm::mat4 get_place_matrix(const raycast_info *rc, unsigned int index) const {
        glm::mat4 m;
        auto rot_axis = glm::vec3{surface_index_to_normal(surface_zp)};

        float step = 1;
        switch (place_mode) {
            case PlaceMode::EighthBlockSnapped: {
                step *= 2;
                // falls through
            }
            case PlaceMode::QuarterBlockSnapped: {
                step *= 2;
                // falls through
            }
            case PlaceMode::HalfBlockSnapped: {
                step *= 2;
                auto pos = rc->intersection;
                m = mat_rotate_mesh(pos, rc->n);
                auto p = m[3];
                auto n = surface_index_to_normal(index ^ 1);
                if (n.x != 0) {
                    p.y = std::round(p.y * step) / step;
                    p.z = std::round(p.z * step) / step;
                } else if (n.y != 0) {
                    p.x = std::round(p.x * step) / step;
                    p.z = std::round(p.z * step) / step;
                } else if (n.z != 0) {
                    p.x = std::round(p.x * step) / step;
                    p.y = std::round(p.y * step) / step;
                }
                m[3] = p;
                break;
            }
            case PlaceMode::BlockSnapped: {
                m = mat_block_face(rc->p, index ^ 1);
                break;
            }
            case PlaceMode::FreeForm: {
                auto pos = rc->intersection;
                m = mat_rotate_mesh(pos, rc->n);
                break;
            }
        }
        m = rotate(m, glm::radians((float) cur_rotate), rot_axis);
        return m;
    }
};

tool *tool::create_add_entity_tool() { return new add_entity_tool; }
