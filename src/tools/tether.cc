#include <epoxy/gl.h>
#include <deque>
#include <sstream>
#include <glm/ext.hpp>

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

extern component_system_manager component_system_man;

extern ImGuiContext* tool_offscreen_context;
extern GLuint render_displays_fbo;

const glm::vec3 fp_item_offset{0.115f, 0.2f, -0.115f };
const float fp_item_scale{ 0.175f };
const glm::quat fp_item_rot{-5.f, 5.f, 5.f, 5.f };

struct tether_tool : tool
{
    c_entity entity;
    raycast_info_world rc;
    raycast_info_world irc;

    std::deque<std::string> net_msg_history {};

    const float MAX_TETHER_HIT = 6.0f;

    enum class CustomizeState {
        CommsInspection,
        CommsOutput,
        CommsFilter,
    } state{CustomizeState::CommsInspection}, next_state{state};

    void pre_use(player *pl) {
        // todo: cast from tool instead of eye?
        if (phys_raycast_world(pl->eye, pl->eye + MAX_TETHER_HIT * pl->dir,
                           phy->rb_controller.get(), phy->dynamicsWorld.get(), &rc)) {
            btRigidBody *ignore = nullptr;
            auto &phys_man = component_system_man.managers.physics_component_man;
            if (phys_man.exists(rc.entity)) {
                ignore = *phys_man.get_instance_data(rc.entity).rigid;
            }
            phys_raycast_world(rc.hitCoord + -pl->dir * 0.0001f, rc.hitCoord + 6.f * -pl->dir,
                               ignore, phy->dynamicsWorld.get(), &irc);
        }
    }

    bool can_use() {
        auto &phys_man = component_system_man.managers.physics_component_man;

        return rc.hit;// && c_entity::is_valid(rc.entity) && phys_man.exists(rc.entity);
    }

    void update() override {
    }

    void use() override {
        if (!can_use())
            return;

        if (phy->tether.is_attached()) {
            phy->tether.detach(phy->dynamicsWorld.get());
        }

        if (c_entity::is_valid(rc.entity)) {
            auto &phys_man = component_system_man.managers.physics_component_man;
            auto p = *phys_man.get_instance_data(rc.entity).rigid;

            phy->tether.attach_to_entity(phy->dynamicsWorld.get(), rc.hitCoord, p, rc.entity);
            phy->tether.attach_to_rb(phy->dynamicsWorld.get(), irc.hitCoord, phy->rb_controller.get());
        }
        else {
            raycast_info_block brc;
            ship->raycast_block(pl.eye, pl.dir, MAX_TETHER_HIT, cross_surface, &brc);

            auto surf = (surface_index)normal_to_surface_index(&brc);
            phy->tether.attach_to_surface(phy->dynamicsWorld.get(), rc.hitCoord, brc.bl, surf);
            phy->tether.attach_to_rb(phy->dynamicsWorld.get(), irc.hitCoord, phy->rb_controller.get());
        }
    }

    void alt_use() override {
        if (!can_use())
            return;

        if (phy->tether.is_attached()) {
            phy->tether.detach(phy->dynamicsWorld.get());
        }
    }

    void preview(frame_data *frame) override {
        auto &rend = component_system_man.managers.renderable_component_man;
        auto &pos_man = component_system_man.managers.position_component_man;

        auto fp_mesh = &asset_man.get_mesh("fp_customize_tool");
        draw_fp_mesh(frame, fp_mesh);

        asset_man.bind_render_textures(0);

        auto fp_screen = &asset_man.get_mesh("fp_customize_tool_screen");
        glUseProgram(screen_shader);
        draw_fp_display_mesh(frame, fp_screen, asset_man.render_textures->array_size - 1);
    }

    void do_offscreen_render() override {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &wire_man = component_system_man.managers.wire_comms_component_man;

        ImGui::SetCurrentContext(tool_offscreen_context);
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
            ImGui::Begin("Tether Tool", nullptr, flags);
            {
                if (!can_use()) {
                    ImGui::Text("Out of range");
                }
                else {
                    ImGui::Text("Distance: %0.2f meters", glm::distance(rc.hitCoord, pl.pos));
                }
            }
            ImGui::End();
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_displays_fbo);
            glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, asset_man.render_textures->texobj, 0,
                asset_man.render_textures->array_size - 1);
            glViewport(0, 0, RENDER_DIM, RENDER_DIM);
            float color[] = { 0, 0.18f, 0.21f, 1 };
            glClearBufferfv(GL_COLOR, 0, color);
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

    void get_description(char *str) override {
        strcpy(str, "Tether tool");
    }

    void unselect() override {
    }

    virtual void cycle_mode() {
    }
};

tool *tool::create_tether_tool() { return new tether_tool(); }
