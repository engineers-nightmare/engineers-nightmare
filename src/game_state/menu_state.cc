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
extern void apply_video_settings();
extern void request_exit();
extern void teardown_chunks();

extern en_settings game_settings;
extern ship_space *ship;
extern SoLoud::Soloud * audio;
extern bool draw_fps, draw_debug_text, draw_debug_chunks, draw_debug_axis, draw_debug_physics;

extern glm::vec3 fp_item_offset;
extern float fp_item_scale;
extern glm::quat fp_item_rot;

struct menu_state : game_state {
    enum class MenuState {
        Main,
        Game,
        Settings,
        Keybinds,
    } state{MenuState::Main};

    bool settings_dirty = false;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    menu_state() {
        state = MenuState::Main;
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            switch (state) {
                case MenuState::Main:
                    set_next_game_state(create_play_state());
                    break;
                case MenuState::Game:
                case MenuState::Settings:
                case MenuState::Keybinds:
                    state = MenuState::Main;
                    break;
            }
        }
    }

    void handle_main_menu() {
        // todo: use a proper state transition system
        if (settings_dirty) {
            save_settings(game_settings);
            apply_video_settings();
            settings_dirty = false;
        }

        ImGui::Begin("", nullptr, menu_flags);
        {
            ImGui::Text("Engineer's Nightmare");
            ImGui::Separator();
            ImGui::Dummy(ImVec2{ 10, 10 });
            if (ImGui::Button("Resume Game")) {
                set_next_game_state(create_play_state());
            }
            ImGui::Dummy(ImVec2{10, 10});
            if (ImGui::Button("Game")) {
                state = MenuState::Game;
            }
            ImGui::Dummy(ImVec2{10, 10});
            if (ImGui::Button("Settings")) {
                state = MenuState::Settings;
            }
            ImGui::Dummy(ImVec2{10, 10});
            if (ImGui::Button("Exit Game")) {
                request_exit();
            }
        }
        ImGui::End();
    }
    void handle_game_menu() {
        ImGui::Begin("", nullptr, menu_flags);
        {
            ImGui::Text("Game");
            ImGui::Separator();
            ImGui::Dummy(ImVec2{10, 10});

            if (ImGui::Button("Save")) {
                // we may want to get clever and pause the world or defer this to
                // a particular time
                save(ship, "save/ship.out");

                set_next_game_state(create_play_state());
            }
            ImGui::Dummy(ImVec2{10, 10});
            if (ImGui::Button("Load")) {
                // we may want to get clever and pause the world or defer this to
                // a particular time
                teardown_chunks();
                delete ship;
                ship = new ship_space();
                load(ship, "save/ship.out");

                set_next_game_state(create_play_state());
            }
        }
        ImGui::End();
    }

    void handle_settings_menu() {
        ImGui::Begin("", nullptr, menu_flags);
        {
            ImGui::Text("Settings");
            ImGui::Separator();

            std::array<const char*, 2> modes{
                    get_enum_description(window_mode::windowed),
                    get_enum_description(window_mode::fullscreen),
            };
            auto mode = game_settings.video.mode;
            settings_dirty |= ImGui::Combo("Window Mode", (int*)&mode, modes.data(), (int)modes.size());
            game_settings.video.mode = mode;

            auto fov = glm::radians(game_settings.video.fov);
            settings_dirty |= ImGui::SliderAngle("Field of View", &fov, 50.0f, 120.0f);
            game_settings.video.fov = glm::degrees(fov);

            bool invert = game_settings.input.mouse_invert == -1.0f;
            settings_dirty |= ImGui::Checkbox("Invert Mouse", &invert);
            game_settings.input.mouse_invert = invert ? -1.0f : 1.0f;

            float vol = audio->getGlobalVolume();
            settings_dirty |= ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f);
            game_settings.audio.global_volume = vol;
            audio->setGlobalVolume(vol);

            ImGui::Separator();
            ImGui::Checkbox("Draw FPS", &draw_fps);
            ImGui::Separator();

            // debug
            bool draw_debug = draw_debug_text && draw_debug_chunks && draw_debug_axis && draw_debug_physics;
            if (ImGui::Checkbox("Draw Debug All", &draw_debug)) {
                draw_debug_text = draw_debug_chunks = draw_debug_axis = draw_debug_physics = draw_debug;
            }
            ImGui::Checkbox("Draw Debug Text", &draw_debug_text);
            ImGui::Checkbox("Draw Chunk Debug", &draw_debug_chunks);
            ImGui::Checkbox("Draw Axis Debug", &draw_debug_axis);
            ImGui::Checkbox("Draw Physics Debug", &draw_debug_physics);

            ImGui::Separator();
            ImGui::SliderFloat3("FP item offset", glm::value_ptr(fp_item_offset), -0.5f, 0.5f);
            ImGui::SliderFloat("FP item scale", &fp_item_scale, 0.f, 1.f);
            ImGui::SliderFloat4("FP item rot", glm::value_ptr(fp_item_rot), -5.f, 5.f);

            ImGui::Dummy(ImVec2{ 10, 10 });
            if (ImGui::Button("Back")) {
                state = MenuState::Main;
            }
        }
        ImGui::End();
    }

    void render(frame_data *frame) override {
        auto io = ImGui::GetIO();

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
            switch (state) {
                case MenuState::Main: {
                    handle_main_menu();
                    break;
                }
                case MenuState::Game: {
                    handle_game_menu();
                    break;
                }
                case MenuState::Settings: {
                    handle_settings_menu();
                    break;
                }
                case MenuState::Keybinds: {
//                    handle_keybinds_menu();
                    break;
                }
            }
        }
    }
};

game_state *game_state::create_menu_state() { return new menu_state; }
