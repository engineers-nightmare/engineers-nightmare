#pragma once

#include <SDL_mouse.h>
#include <soloud.h>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../config.h"
#include "../save.h"
#include "../load.h"
#include "../ship_space.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);
extern void warp_mouse_to_center_screen();
extern bool window_has_focus();
extern void apply_video_settings();
extern void request_exit();
extern void new_imgui_frame();
extern void teardown_chunks();

extern en_settings game_settings;
extern ship_space *ship;
extern SoLoud::Soloud * audio;
extern bool draw_fps, draw_debug_text, draw_debug_chunks, draw_debug_axis, draw_debug_physics;

extern ImGuiContext *default_context;

extern glm::vec3 fp_item_offset;
extern float fp_item_scale;
extern glm::quat fp_item_rot;

struct customize_entity_state : game_state {
    enum class MenuState {
        Comms,
    } state{MenuState::Comms};

    c_entity entity;

    bool settings_dirty = false;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_state(c_entity e) : entity(e) {
        warp_mouse_to_center_screen();
        state = MenuState::Comms;
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            switch (state) {
                case MenuState::Comms:
                    set_next_game_state(create_play_state());
                    break;
            }
        }
    }

    void rebuild_ui() override {
    }

    void update(float dt) override {
        if (window_has_focus() && SDL_GetRelativeMouseMode() == SDL_TRUE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }

    void handle_comms_menu() {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &wire_man = component_system_man.managers.wire_comms_component_man;

        auto type = type_man.get_instance_data(entity);
        auto wire = wire_man.get_instance_data(entity);

        ImGui::Begin("Comms Configuration", nullptr, menu_flags);
        {
            if (!c_entity::is_valid(entity)) {
                ImGui::Text("Invalid Entity %d", entity.id);
            }
            else {
                auto *label = new char[256];

                if (*wire.label != nullptr) {
                    strcpy(label, *wire.label);
                }
                else {
                    strcpy(label, "");
                }

                ImGui::Text("%s", *type.name);
                ImGui::Separator();
                ImGui::Dummy(ImVec2{10, 10});
                ImGui::InputText("Label", label, 256, ImGuiInputTextFlags_AutoSelectAll);
                ImGui::Dummy(ImVec2{10, 10});
                if (ImGui::Button("Finish")) {
                    set_next_game_state(create_play_state());
                }

                delete[] *wire.label;
                *wire.label = label;
            }
        }
        ImGui::End();
    }

    void render(frame_data *frame) override {
        ImGui::SetCurrentContext(default_context);
        new_imgui_frame();

        auto io = ImGui::GetIO();

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
            switch (state) {
                case MenuState::Comms: {
                    handle_comms_menu();
                    break;
                }
            }
        }

        ImGui::Render();
    }
};
