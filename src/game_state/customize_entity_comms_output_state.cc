#include <SDL_mouse.h>
#include <vector>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../component/component_system_manager.h"
#include "../entity_utils.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);

extern component_system_manager component_system_man;
extern std::unordered_map<c_entity, std::vector<c_entity>> entity_families;

struct customize_entity_comms_output_state : game_state {
    c_entity entity;

    std::vector<output_ui_state> entity_labels;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    std::vector<output_ui_state> get_entity_labels(c_entity entity) {
        std::vector<output_ui_state> outputs{};

        auto &wire_man = component_system_man.managers.wire_comms_component_man;
        auto &type_man = component_system_man.managers.type_component_man;
        if (wire_man.exists(entity)) {
            auto wire = wire_man.get_instance_data(entity);
            auto type = type_man.get_instance_data(entity);

            outputs.emplace_back();
            auto &f = outputs.back();
            f.entity = entity;
            f.component_name = *type.name;
            if (*wire.label) {

                strcpy(f.output.data(), *wire.label);
            } else {
                f.output[0] = '\0';
            }
        }

        for (auto e : entity_families[entity]) {
            auto f = get_entity_labels(e);
            outputs.insert(outputs.end(), f.begin(), f.end());
        }

        return outputs;
    }

    explicit customize_entity_comms_output_state(c_entity e) : entity(e) {
        entity_labels = get_entity_labels(get_root_parent(entity));
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

                    ImGui::LabelText("", "%s", "Entity");
                    ImGui::SameLine();
                    ImGui::LabelText("", "%s", "");
                    ImGui::SameLine();
                    ImGui::LabelText("", "%s", "Output Label");

                    int id = 0;
                    for (auto &kvp : entity_labels) {
                        ImGui::PushID(id++);
                        ImGui::LabelText("", "%s", *type_man.get_instance_data(kvp.entity).name);

                        ImGui::SameLine();
                        ImGui::LabelText("", "%s", kvp.component_name.c_str());

                        ImGui::SameLine();
                        ImGui::InputText("###output", kvp.output.data(), 256);
                        ImGui::PopID();
                    }

                    ImGui::Dummy(ImVec2{10, 10});


                    ImGui::Dummy(ImVec2{10, 10});
                    if (ImGui::Button("Save")) {
                        for (auto &kvp : entity_labels) {
                            auto &wire_man = component_system_man.managers.wire_comms_component_man;
                            auto wire = wire_man.get_instance_data(kvp.entity);

                            free((void *) *wire.label);
                            *wire.label = strdup(kvp.output.data());
                        }
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
