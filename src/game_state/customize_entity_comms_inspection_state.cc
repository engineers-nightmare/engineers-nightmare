#include <SDL_mouse.h>
#include <vector>
#include <unordered_map>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../component/component_system_manager.h"
#include "../entity_utils.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);

extern ship_space *ship;

extern component_system_manager component_system_man;
extern std::unordered_map<c_entity, std::vector<c_entity>> entity_families;

struct customize_entity_comms_inspection_state : game_state {
    c_entity entity;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_comms_inspection_state(c_entity e) : entity(e) {
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            set_next_game_state(create_play_state());
        }
    }

    void post_label(c_entity entity, type_component_manager &type_man, wire_comms_component_manager &wire_man) {
        if (wire_man.exists(entity)) {
            ImGui::TextUnformatted(*(type_man.get_instance_data(entity).name));
        }

        for (auto e : entity_families[entity]) {
            post_label(e, type_man, wire_man);
        }
    }

    void update(float dt) override {
    }

    void render(frame_data *frame) override {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &wire_man = component_system_man.managers.wire_comms_component_man;

        auto io = ImGui::GetIO();

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
            ImGui::Begin("Comms Inspection", nullptr, menu_flags);
            {
                if (!c_entity::is_valid(entity)) {
                    ImGui::Text("Invalid Entity %d", entity.id);
                } else {
                    auto cur = get_root_parent(entity);
                    post_label(cur, type_man, wire_man);

                    if (ImGui::Button("Back")) {
                        set_next_game_state(create_play_state());
                    }
                }
            }
            ImGui::End();
        }
    }
};

game_state *game_state::create_customize_entity_comms_inspection_state(c_entity e) { return new customize_entity_comms_inspection_state(e); }
