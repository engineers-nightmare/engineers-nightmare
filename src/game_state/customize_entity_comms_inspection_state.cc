#include <SDL_mouse.h>
#include <vector>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../component/component_system_manager.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);

extern void warp_mouse_to_center_screen();
extern bool window_has_focus();
extern void new_imgui_frame();

extern ImGuiContext *default_context;

extern component_system_manager component_system_man;

struct customize_entity_comms_inspection_state : game_state {
    ship_space *ship;
    c_entity entity;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_comms_inspection_state(ship_space *ship, c_entity e) : ship(ship), entity(e) {
        warp_mouse_to_center_screen();
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            set_next_game_state(create_play_state());
        }
    }

    void rebuild_ui() override {
    }

    void update(float dt) override {
        if (window_has_focus() && SDL_GetRelativeMouseMode() == SDL_TRUE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }

    void render(frame_data *frame) override {
        ImGui::SetCurrentContext(default_context);
        new_imgui_frame();

        auto io = ImGui::GetIO();

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
            ImGui::Begin("Comms Inspection", nullptr, menu_flags);
            {
                if (!c_entity::is_valid(entity)) {
                    ImGui::Text("Invalid Entity %d", entity.id);
                }
                else {
                    if (ImGui::Button("Back")) {
                        set_next_game_state(create_play_state());
                    }
                }
            }
            ImGui::End();
        }

        ImGui::Render();
    }
};

game_state *game_state::create_customize_entity_comms_inspection_state(ship_space *ship, c_entity e) { return new customize_entity_comms_inspection_state(ship, e); }
