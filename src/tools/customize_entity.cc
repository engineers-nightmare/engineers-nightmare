#include <epoxy/gl.h>
#include <deque>
#include <sstream>

#include "tools.h"
#include "../asset_manager.h"
#include "../common.h"
#include "../ship_space.h"
#include "../mesh.h"
#include "../player.h"
#include "../component/component_system_manager.h"
#include "../imgui/imgui.h"
#include "../imgui_impl_sdl_gl3.h"
#include "../game_state.h"

extern GLuint simple_shader;
extern GLuint highlight_shader;
extern GLuint screen_shader;

extern ship_space *ship;

extern physics *phy;

extern asset_manager asset_man;
extern player pl;
extern glm::mat4 get_fp_item_matrix();

extern void destroy_entity(c_entity e);

extern component_system_manager component_system_man;

extern std::array<ImGuiContext*, 3> offscreen_contexts;
extern GLuint render_displays_fbo;

extern void set_next_game_state(game_state *s);

const glm::vec3 fp_item_offset{0.115f, 0.2f, -0.115f };
const float fp_item_scale{ 0.175f };
const glm::quat fp_item_rot{-5.f, 5.f, 5.f, 5.f };

struct customize_entity_tool : tool
{
    c_entity entity;
    raycast_info_world rc;

    std::deque<std::string> net_msg_history {};

    enum class CustomizeState {
        CommsInspection,
        CommsOutput,
        CommsFilter,
    } state{CustomizeState::CommsInspection}, next_state{state};

    void pre_use(player *pl) {
        // todo: cast from tool instead of eye?
        phys_raycast_world(pl->eye, pl->eye + 2.f * pl->dir,
                           phy->rb_controller.get(), phy->dynamicsWorld.get(), &rc);
    }

    void switch_state(CustomizeState new_state) {
        switch (new_state) {
            case CustomizeState::CommsInspection: {
                break;
            }
            case CustomizeState::CommsOutput: {
                break;
            }
            case CustomizeState::CommsFilter: {
                break;
            }
        }
        state = new_state;
    }

    void set_next_state(CustomizeState new_state) {
        next_state = new_state;
    }

    bool can_use() {
        bool valid_hit = !(!rc.hit || !c_entity::is_valid(entity));

        auto &wire_man = component_system_man.managers.wire_comms_component_man;

        // tool is usable if entity contains something configurable by tool
        return valid_hit && wire_man.exists(entity);
    }

    void update(ship_space *ship) override {
        if (next_state != state) {
            switch_state(next_state);
        }

        if (c_entity::is_valid(entity)) {
            auto &wire_man = component_system_man.managers.wire_comms_component_man;
            if (wire_man.exists(entity)) {
                auto wire = wire_man.get_instance_data(entity);
                auto net = ship->comms_networks[*wire.network];

                for (auto const &m : net.read_buffer) {
                    if (wire_man.exists(m.originator)) {
                        auto l = *wire_man.get_instance_data(m.originator).label;
                        std::string sender = l ? l : "";
                        auto type = get_enum_description(m.type);
                        std::stringstream s;
                        s << sender << ": " << type << " - " << m.data;
                        net_msg_history.emplace_back(s.str());
                    }
                }

                while (net_msg_history.size() > 3) {
                    net_msg_history.pop_front();
                }
            }
        }
    }

    void use(ship_space *ship) override {
        if (!can_use())
            return;

        switch (state) {
            case CustomizeState::CommsInspection: {
                set_next_game_state(game_state::create_customize_entity_comms_inspection_state(ship, entity));
                break;
            }
            case CustomizeState::CommsOutput: {
                set_next_game_state(game_state::create_customize_entity_comms_output_state(entity));
                break;
            }
            case CustomizeState::CommsFilter: {
                set_next_game_state(game_state::create_customize_entity_comms_filter_state(entity));
                break;
            }
        }
    }

    void preview(frame_data *frame) override {
        auto &rend = component_system_man.managers.renderable_component_man;
        auto &pos_man = component_system_man.managers.position_component_man;

        if (entity != rc.entity) {
            if (c_entity::is_valid(entity) && rend.exists(entity)) {
                *rend.get_instance_data(entity).draw = true;
            }

            pl.ui_dirty = true;    // TODO: untangle this.
            entity = rc.entity;

            if (can_use() && c_entity::is_valid(entity) && rend.exists(entity)) {
                *rend.get_instance_data(entity).draw = false;
            }
        }

        if (can_use() && c_entity::is_valid(entity) && rend.exists(entity)) {
            auto mat = frame->alloc_aligned<mesh_instance>(1);
            mat.ptr->world_matrix =  *pos_man.get_instance_data(entity).mat;
            mat.bind(1, frame);

            auto inst = rend.get_instance_data(entity);
            auto mesh = asset_man.get_mesh(*inst.mesh);
            glUseProgram(highlight_shader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            draw_mesh(mesh.hw);
            glDisable(GL_BLEND);
            glUseProgram(simple_shader);
        }

        auto fp_mesh = &asset_man.get_mesh("fp_customize_tool");
        draw_fp_mesh(frame, fp_mesh);

        asset_man.bind_render_textures(0);

        auto fp_screen = &asset_man.get_mesh("fp_customize_tool_screen");
        glUseProgram(screen_shader);
        draw_fp_display_mesh(frame, fp_screen, 2);
    }

    void do_offscreen_render() override {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &wire_man = component_system_man.managers.wire_comms_component_man;

        ImGui::SetCurrentContext(offscreen_contexts[2]);
        const int scale = 6;
        ImGui_ImplSdlGL3_NewFrameOffscreen(RENDER_DIM / scale, RENDER_DIM / scale);

        auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoInputs;
        // center on screen
        // todo: modify NewFrame() to allow us to use this properly
        ImGui::SetNextWindowPos(ImVec2{ RENDER_DIM / scale / 2, RENDER_DIM / scale / 2 }, 0, ImVec2{ 0.5f, 0.5f });
        {
            ImGui::Begin("Customize Tool", nullptr, flags);
            {
                if (!c_entity::is_valid(entity)) {
                    ImGui::Text("Scanning...");
                }
                else {
                    ImGui::Text("Found:");
                    auto name = type_man.get_instance_data(entity).name;
                    ImGui::Text("%s", *name);

                    if (wire_man.exists(entity)) {

                        auto label = *(wire_man.get_instance_data(entity).label);
                        if (label != nullptr && strcmp(label, "") != 0) {
                            ImGui::Text("%s", label);
                        } else {
                            ImGui::Text("No label");
                        }

                        for (auto const &h : net_msg_history) {
                            ImGui::TextUnformatted(h.c_str());
                        }
                    }
                }
            }
            ImGui::End();
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_displays_fbo);
            glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, asset_man.render_textures->texobj, 0, 2);
            glViewport(0, 0, RENDER_DIM, RENDER_DIM);
            glClearColor(0, 0.18f, 0.21f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
        }
    }

    void draw_fp_mesh(frame_data *frame, const mesh_data *mesh) const {
        auto m = mat4_cast(normalize(pl.rot));
        auto right = glm::vec3(m[0]);
        auto up = glm::vec3(m[1]);

        auto fpmat = frame->alloc_aligned<mesh_instance>(1);
        fpmat.ptr->world_matrix = scale(mat_position(pl.eye + pl.dir * fp_item_offset.y + right * fp_item_offset.x + up * fp_item_offset.z), glm::vec3(fp_item_scale)) * m * mat4_cast(
                normalize(fp_item_rot));
        fpmat.ptr->color = glm::vec4(1.f, 1.f, 1.f, 1.f);
        fpmat.bind(1, frame);
        draw_mesh(mesh->hw);
    }

    void draw_fp_display_mesh(frame_data *frame, const mesh_data *mesh, int layer) const {
        auto m = mat4_cast(normalize(pl.rot));
        auto right = glm::vec3(m[0]);
        auto up = glm::vec3(m[1]);

        auto fpmat = frame->alloc_aligned<display_mesh_instance>(1);
        fpmat.ptr->world_matrix = scale(mat_position(pl.eye + pl.dir * fp_item_offset.y + right * fp_item_offset.x + up * fp_item_offset.z), glm::vec3(fp_item_scale)) * m * mat4_cast(
                normalize(fp_item_rot));
        fpmat.ptr->material = layer;
        fpmat.bind(1, frame);
        draw_mesh(mesh->hw);
    }

    const char * get_state_description() {
        switch (state) {
            case CustomizeState::CommsInspection: {
                return "Inspection";
                break;
            }
            case CustomizeState::CommsOutput: {
                return "Output";
                break;
            }
            case CustomizeState::CommsFilter: {
                return "Filter";
                break;
            }
        }

        return "";
    }

    void get_description(char *str) override {
        auto &type_man = component_system_man.managers.type_component_man;
        if (can_use()) {
            auto type = type_man.get_instance_data(entity);
            sprintf(str, "Customize %s - %s", *type.name, get_state_description());
        }
        else {
            strcpy(str, "Customize entity tool");
        }
    }

    void unselect() override {
        if (c_entity::is_valid(entity)) {
            auto &rend = component_system_man.managers.renderable_component_man;
            *rend.get_instance_data(entity).draw = true;
            entity = c_entity{};
        }
    }

    virtual void cycle_mode() {
        switch (state) {
            case CustomizeState::CommsInspection: {
                set_next_state(CustomizeState::CommsOutput);
                break;
            }
            case CustomizeState::CommsOutput: {
                set_next_state(CustomizeState::CommsFilter);
                break;
            }
            case CustomizeState::CommsFilter: {
                set_next_state(CustomizeState::CommsInspection);
                break;
            }
        }
    }
};

tool *tool::create_customize_entity_tool() { return new customize_entity_tool(); }
