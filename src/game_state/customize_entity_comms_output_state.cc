#include <SDL_mouse.h>
#include <vector>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../component/component_system_manager.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);

extern component_system_manager component_system_man;
extern std::unordered_map<c_entity, std::vector<c_entity>> entity_families;

struct customize_entity_comms_output_state : game_state {
    c_entity entity;

    char entity_label[256]{};

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_comms_output_state(c_entity e) : entity(e) {
        auto &wire_man = component_system_man.managers.wire_comms_component_man;
        auto wire = wire_man.get_instance_data(entity);
        strcpy(entity_label, (*wire.label) ? *wire.label : "");
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            set_next_game_state(create_play_state());
        }
    }

    void render(frame_data *frame) override {
        auto io = ImGui::GetIO();

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
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
                    ImGui::Text("Set Output Label - %s", *type.name);
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
                        set_next_game_state(create_play_state());
                    }
                }
            }
            ImGui::End();
        }
    }
};

game_state *game_state::create_customize_entity_comms_output_state(c_entity e) { return new customize_entity_comms_output_state(e); }
