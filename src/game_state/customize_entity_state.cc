#include <SDL_mouse.h>
#include <soloud.h>
#include <vector>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../config.h"
#include "../component/component_system_manager.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);
extern void warp_mouse_to_center_screen();
extern bool window_has_focus();
extern void apply_video_settings();
extern void request_exit();
extern void new_imgui_frame();
extern void teardown_chunks();
extern std::vector<std::pair<const std::string, std::string>> get_filters(c_entity);
extern void update_filter(c_entity entity, std::string const&, char[256]);

extern en_settings game_settings;
extern ship_space *ship;
extern SoLoud::Soloud * audio;
extern bool draw_fps, draw_debug_text, draw_debug_chunks, draw_debug_axis, draw_debug_physics;

extern ImGuiContext *default_context;

extern component_system_manager component_system_man;

extern glm::vec3 fp_item_offset;
extern float fp_item_scale;
extern glm::quat fp_item_rot;

struct customize_entity_state : game_state {
    enum class CustomizeState {
        Main,
        CommsOutput,
        CommsFilter,
    } state{CustomizeState::Main}, next_state{state};

    c_entity entity;

    char entity_label[256]{};
    std::vector<std::pair<const std::string, std::string>> comp_name_to_filter_name;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_state(c_entity e) : entity(e) {
        warp_mouse_to_center_screen();
        state = CustomizeState::Main;
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            switch (state) {
                case CustomizeState::CommsOutput:
                case CustomizeState::CommsFilter:
                    set_next_state(CustomizeState::Main);
                    break;
                case CustomizeState::Main:
                    set_next_game_state(create_play_state());
                    break;
            }
        }
    }

    void rebuild_ui() override {
    }

    void switch_state(CustomizeState new_state) {
        switch (new_state) {
            case CustomizeState::CommsOutput: {
                auto &wire_man = component_system_man.managers.wire_comms_component_man;
                auto wire = wire_man.get_instance_data(entity);
                strcpy(entity_label, (*wire.label) ? *wire.label : "");
                break;
            }
            case CustomizeState::CommsFilter: {
                comp_name_to_filter_name = get_filters(entity);
                break;
            }
            default:
                break;
        }
        state = new_state;
    }

    void set_next_state(CustomizeState new_state) {
        next_state = new_state;
    }

    void update(float dt) override {
        if (window_has_focus() && SDL_GetRelativeMouseMode() == SDL_TRUE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }

        if (next_state != state) {
            switch_state(next_state);
        }
    }

    void handle_main_state() {
        ImGui::Begin("Main", nullptr, menu_flags);
        {
            if (!c_entity::is_valid(entity)) {
                ImGui::Text("Invalid Entity %d", entity.id);
            }
            else {
                auto &type_man = component_system_man.managers.type_component_man;
                auto type = type_man.get_instance_data(entity);

                ImGui::Text("%s", *type.name);
                ImGui::Separator();
                ImGui::Dummy(ImVec2{10, 10});
                if (ImGui::Button("Output")) {
                    set_next_state(CustomizeState::CommsOutput);
                }
                else if (ImGui::Button("Filters")) {
                    set_next_state(CustomizeState::CommsFilter);
                }
                else if (ImGui::Button("Back")) {
                    set_next_game_state(create_play_state());
                }
            }
        }
        ImGui::End();
    }

    void handle_comms_output_state() {
        ImGui::Begin("Comms Output", nullptr, menu_flags);
        {
            if (!c_entity::is_valid(entity)) {
                ImGui::Text("Invalid Entity %d", entity.id);
            }
            else {
                auto &type_man = component_system_man.managers.type_component_man;
                auto type = type_man.get_instance_data(entity);

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere(0);
                }
                ImGui::Text("%s", *type.name);
                ImGui::Separator();
                ImGui::Dummy(ImVec2{10, 10});
                bool save_label = ImGui::InputText("Comms Label", entity_label, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::Dummy(ImVec2{10, 10});
                save_label = ImGui::Button("Save") || save_label;
                if (save_label) {
                    auto &wire_man = component_system_man.managers.wire_comms_component_man;
                    auto wire = wire_man.get_instance_data(entity);

                    free((void *) *wire.label);
                    *wire.label = strdup(entity_label);
                }

                if (ImGui::Button("Back")) {
                    set_next_state(CustomizeState::Main);
                }
            }
        }
        ImGui::End();
    }

    void handle_comms_filter_state() {
        ImGui::Begin("Comms Filter", nullptr, menu_flags);
        {
            if (!c_entity::is_valid(entity)) {
                ImGui::Text("Invalid Entity %d", entity.id);
            }
            else {
                auto &type_man = component_system_man.managers.type_component_man;
                auto type = type_man.get_instance_data(entity);

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere(0);
                }
                ImGui::Text("%s", *type.name);
                ImGui::Separator();
                ImGui::Dummy(ImVec2{10, 10});
                for (auto && kvp: comp_name_to_filter_name) {
                    auto comp = kvp.first;
                    char filter[256];
                    strcpy(filter, kvp.second.c_str());

                    bool save_filter = ImGui::InputText(comp.c_str(), filter, 256,
                                                       ImGuiInputTextFlags_AutoSelectAll |
                                                       ImGuiInputTextFlags_EnterReturnsTrue);
                    kvp.second = filter;
                    ImGui::Dummy(ImVec2{10, 10});
                    save_filter = ImGui::Button("Save") || save_filter;
                    if (save_filter) {
                        update_filter(entity, comp, filter);
                    }
                }

                if (ImGui::Button("Back")) {
                    set_next_state(CustomizeState::Main);
                }
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
                case CustomizeState::Main:
                    handle_main_state();
                    break;
                case CustomizeState::CommsOutput:
                    handle_comms_output_state();
                    break;
                case CustomizeState::CommsFilter:
                    handle_comms_filter_state();
                    break;
            }
        }

        ImGui::Render();
    }
};

game_state *game_state::create_customize_entity_state(c_entity e) { return new customize_entity_state(e); }
