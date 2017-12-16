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
#include "../imgui/imgui.h"
#include "../imgui_impl_sdl_gl3.h"


extern GLuint overlay_shader;
extern GLuint simple_shader;

extern ship_space *ship;

extern asset_manager asset_man;
extern component_system_manager component_system_man;
extern player pl;

extern std::vector<std::string> entity_names;
extern std::unordered_map<std::string, entity_data> entity_stubs;

extern frame_info frame_info;
extern frame_data *frame;
extern ImGuiContext *default_context;

void get_window_size(unsigned *w, unsigned *h);

struct add_entity_tool : tool {
    const unsigned rotate_tick_rate = 5; // hz
    double last_rotate_time = 0;
    unsigned cur_rotate = 0;

    unsigned entity_name_index = 0;

    struct mesh_grid {
        unsigned entity_name_index{};
        glm::ivec4 grid_rect{};
    };
    std::vector<mesh_grid> mesh_grids;

    enum class add_entity_state {
        place_entity,
        pick_entity,
    } state{add_entity_state::place_entity};

    static void draw_entity_in_grid(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
        if (!cmd->UserCallbackData) {
            return;
        }
        auto mg = (mesh_grid*)cmd->UserCallbackData;

        glm::mat4 vm;
        glm::mat4 pm;
        vm = glm::lookAt(glm::vec3{1.5f, 0, 0.5f}, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1});
        pm = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 20.0f);

        auto camera_params = frame->alloc_aligned<per_camera_params>(1);

        camera_params.ptr->view_proj_matrix = pm * vm;
        camera_params.ptr->aspect = 1.0f;
        camera_params.bind(0, frame);

        auto name = entity_names[mg->entity_name_index];
        auto render = entity_stubs[name].get_component<renderable_component_stub>();
        auto mesh = asset_man.get_mesh(render->mesh);
        auto material = asset_man.get_world_texture_index(render->material);

        auto params = frame->alloc_aligned<mesh_instance>(1);
        auto m = mat_rotate_mesh({0, 0, 0}, {1, 0, 1});
        auto r = glm::rotate(SDL_GetTicks() / 5000.0f, glm::vec3{0, 0, 1});
        params.ptr->world_matrix = m * r;
        params.ptr->material = material;
        params.bind(1, frame);

        glUseProgram(simple_shader);
        bool depth_enabled = glIsEnabled(GL_DEPTH_TEST);
        if (!depth_enabled) {
            glEnable(GL_DEPTH_TEST);
        }
        glScissor(mg->grid_rect.x, mg->grid_rect.y, mg->grid_rect.z, mg->grid_rect.w);
        glViewport(mg->grid_rect.x, mg->grid_rect.y, mg->grid_rect.z, mg->grid_rect.w);
        draw_mesh(mesh.hw);
        glUseProgram(simple_shader);
        if (!depth_enabled) {
            glDisable(GL_DEPTH_TEST);
        }
    }

    bool get_entity_from_browser(unsigned *entity_index) {
        mesh_grids.resize(0);
        const unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

        unsigned w, h;
        get_window_size(&w, &h);

        ImGui::SetCurrentContext(default_context);
        ImGui_ImplSdlGL3_NewFrame(w, h);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, w, h);
        ImGui::Begin("", nullptr, menu_flags);

        ImGui::Text("Entity Browser");
        const int columns = 3;
        const int dim = 200;
        const int margin = 5;
        ImGui::Dummy({columns * (dim + 10), 1});
        ImGui::Columns(columns, nullptr, false);

        auto clicked = false;

        for (unsigned index = 0; index < entity_names.size(); ++index) {
// todo: use this when placeable is merged in
//                if (!entity_stubs[entity_names[index]].get_component<placeable_component_stub>()) {
//                    continue;
//                }
            if (ImGui::Button("", {dim + 2 * margin, dim + 2 * margin})) {
                *entity_index = index;
                clicked = true;
            }
            auto height = ImGui::GetIO().DisplaySize.y;
            glm::vec2 pos = ImGui::GetCursorScreenPos();
            pos.x += margin;
            pos.y = height - pos.y + margin;

            mesh_grids.push_back({index, {pos, dim, dim}});
            ImGui::NextColumn();
        }
        ImGui::Columns(1);

        auto list = ImGui::GetWindowDrawList();
        for (auto &&iv : mesh_grids) {
            list->AddCallback(draw_entity_in_grid, (void*)&iv);
        }
        ImGui::End();
        ImGui::Render();

        return clicked;
    }

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

    void select() override {
        state = add_entity_state::place_entity;
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
    }

    // press to rotate once, hold to rotate continuously
    void alt_use(raycast_info *rc) override {
        auto rotate = get_rotate();

        cur_rotate += rotate;
        cur_rotate %= 360;
    }

    void long_use(raycast_info *rc) override {
    }

    // hold to rotate continuously, press to rotate once
    void long_alt_use(raycast_info *rc) override {
        auto rotate = get_rotate();

        if (frame_info.elapsed >= last_rotate_time + 1.0 / rotate_tick_rate) {
            cur_rotate += rotate;
            cur_rotate %= 360;
            last_rotate_time = frame_info.elapsed;
        }
    }

    void cycle_mode() override {
//        do {
//            entity_name_index++;
//            if (entity_name_index >= entity_names.size()) {
//                entity_name_index = 0;
//            }
//        } while (!entity_stubs[entity_names[entity_name_index]].get_component<placeable_component_stub>());
        state = add_entity_state::pick_entity;
    }

    void preview(raycast_info *rc, frame_data *frame) override {
        switch (state) {
            case add_entity_state::pick_entity: {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                ImGui::CaptureMouseFromApp(true);
                unsigned index{0};
                if (get_entity_from_browser(&index)) {
                    entity_name_index = index;

                    auto rotate = get_rotate();
                    if (rotate == 0) {
                        cur_rotate = 0;
                    } else {
                        cur_rotate += rotate / 2;
                        cur_rotate = cur_rotate - (cur_rotate % rotate);
                    }
                    state = add_entity_state::place_entity;
                }
                break;
            }
            case add_entity_state::place_entity: {
                if (!can_use(rc))
                    return;

                auto index = normal_to_surface_index(rc);
                auto render = entity_stubs[entity_names[entity_name_index]].get_component<renderable_component_stub>();

                glm::mat4 m = get_place_matrix(rc, index);

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
                break;
            }
        }
    }

    void get_description(char *str) override {
        auto name = entity_names[entity_name_index];

        sprintf(str, "Place %s", name.c_str());
    }

    glm::mat4 get_place_matrix(const raycast_info *rc, unsigned int index) const {
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
                auto pos = rc->intersection;
                m = mat_rotate_mesh(pos, rc->n);
                m[3].x = std::round(m[3].x * step) / step;
                m[3].y = std::round(m[3].y * step) / step;
                m[3].z = std::round(m[3].z * step) / step;
                break;
            }
            case placement::full_block_snapped: {
                m = mat_block_face(rc->p, index ^ 1);
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
