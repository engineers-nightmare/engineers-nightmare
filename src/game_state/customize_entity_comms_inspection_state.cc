#include <SDL_mouse.h>
#include <vector>
#include <unordered_map>

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
    std::unordered_map<c_entity, std::vector<c_entity>> entity_families;

    unsigned menu_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    explicit customize_entity_comms_inspection_state(ship_space *ship, c_entity e) : ship(ship), entity(e) {
        warp_mouse_to_center_screen();
    }

    void handle_input() override {
        if (get_input(action_menu)->just_active) {
            set_next_game_state(create_play_state());
        }
    }

    void get_entity_hierarchy() {
        entity_families.clear();
        auto &type_man = component_system_man.managers.type_component_man;
        for (auto i = 0u; i < type_man.buffer.num; i++) {
            entity_families[type_man.instance_pool.entity[i]] = {};
        }
        auto &par_man = component_system_man.managers.parent_component_man;
        for (auto i = 0u; i < par_man.buffer.num; i++) {
            entity_families[par_man.instance_pool.parent[i]].emplace_back(par_man.instance_pool.entity[i]);
        }

        for (auto &i : entity_families) {
            std::sort(i.second.begin(), i.second.end());
        }
    }

    c_entity get_root_parent(c_entity entity, parent_component_manager &par_man) {
        auto e = entity;
        while (par_man.exists(e)) {
            e = *par_man.get_instance_data(e).parent;
        }

        return e;
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
        if (window_has_focus() && SDL_GetRelativeMouseMode() == SDL_TRUE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }

        get_entity_hierarchy();
    }

    void render(frame_data *frame) override {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &wire_man = component_system_man.managers.wire_comms_component_man;
        auto &par_man = component_system_man.managers.parent_component_man;

        auto io = ImGui::GetIO();

        // center on screen
        ImGui::SetNextWindowPos(ImVec2{ io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f }, 0, ImVec2{ 0.5f, 0.5f });
        {
            ImGui::Begin("Comms Inspection", nullptr, menu_flags);
            {
                if (!c_entity::is_valid(entity)) {
                    ImGui::Text("Invalid Entity %d", entity.id);
                } else {
                    auto cur = get_root_parent(entity, par_man);
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

game_state *game_state::create_customize_entity_comms_inspection_state(ship_space *ship, c_entity e) { return new customize_entity_comms_inspection_state(ship, e); }
