#include <SDL_mouse.h>
#include <vector>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../component/component_system_manager.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);

extern std::vector<std::pair<const std::string, std::array<char, 256>>> get_filters(c_entity);
extern void update_filter(c_entity entity, std::string const&, std::array<char, 256> const&);

extern component_system_manager component_system_man;

struct customize_entity_comms_filter_state : game_state {
    c_entity entity;

    std::vector<std::pair<const std::string, std::array<char, 256>>> comp_name_to_filter_name;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_comms_filter_state(c_entity e) : entity(e) {
        comp_name_to_filter_name = get_filters(entity);
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
                    ImGui::Text("Set Input Filters - %s", *type.name);
                    ImGui::Separator();
                    ImGui::Dummy(ImVec2{10, 10});
                    for (auto &kvp : comp_name_to_filter_name) {
                        auto comp = kvp.first.c_str();
                        ImGui::InputText(comp, kvp.second.data(), 256);
                    }

                    ImGui::Dummy(ImVec2{10, 10});
                    if (ImGui::Button("Save")) {
                        for (auto &kvp : comp_name_to_filter_name) {
                            update_filter(entity, kvp.first, kvp.second);
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

game_state *game_state::create_customize_entity_comms_filter_state(c_entity e) { return new customize_entity_comms_filter_state(e); }
