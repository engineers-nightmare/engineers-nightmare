#include <SDL_mouse.h>
#include <soloud.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext.hpp>

#include "../game_state.h"
#include "../imgui/imgui.h"
#include "../input.h"
#include "../config.h"
#include "../save.h"
#include "../load.h"
#include "../component/c_entity.h"
#include "../component/component_system_manager.h"
#include "../tools/tools.h"
#include "../text.h"
#include "../ship_space.h"
#include "../entity_utils.h"
#include "../projectile/projectile.h"

extern action const* get_input(en_action a);
extern void set_next_game_state(game_state *s);
extern void warp_mouse_to_center_screen();
extern bool window_has_focus();
extern void apply_video_settings();
extern void request_exit();
extern void add_text_with_outline(char const *, float , float, float = 1, float = 1, float = 1);

extern sprite_metrics unlit_ui_slot_sprite, lit_ui_slot_sprite;

extern component_system_manager component_system_man;
extern std::array<tool*, 9> tools;

extern player pl;
extern physics *phy;
extern text_renderer *text;
extern sprite_renderer *ui_sprites;
extern en_settings game_settings;
extern ship_space *ship;
extern SoLoud::Soloud * audio;
extern projectile_linear_manager proj_man;

extern bool draw_fps, draw_debug_text, draw_debug_chunks, draw_debug_axis, draw_debug_physics;

extern glm::vec3 fp_item_offset;
extern float fp_item_scale;
extern glm::quat fp_item_rot;

struct play_state : game_state {
    c_entity use_entity;

    play_state() = default;

    void rebuild_ui() override {
        auto &type_man = component_system_man.managers.type_component_man;
        auto &switch_man = component_system_man.managers.switch_component_man;
        auto &surf_man = component_system_man.managers.surface_attachment_component_man;

        float w = 0;
        float h = 0;
        char buf[256];
        char buf2[512];

        {
            /* Tool name down the bottom */
            tool *t = tools[pl.active_tool_slot];

            if (t) {
                t->get_description(buf);
            }
            else {
                strcpy(buf, "(no tool)");
            }
        }

        text->measure(".", &w, &h);
        add_text_with_outline(".", -w/2, -w/2);

        auto bind = game_settings.bindings.bindings.find(action_use_tool);
        auto key = lookup_key((*bind).second.binds.inputs[0]);
        sprintf(buf2, "%s: %s", key, buf);
        text->measure(buf2, &w, &h);
        add_text_with_outline(buf2, -w/2, -360);

        /* Use key affordance */
        bind = game_settings.bindings.bindings.find(action_use);
        key = lookup_key((*bind).second.binds.inputs[0]);
        char const *pre = nullptr;
        if (c_entity::is_valid(use_entity)) {
            if (!surf_man.exists(use_entity) ||
                !*(surf_man.get_instance_data(use_entity)).attached) {
                pre = "Remove";
            }
            else if (switch_man.exists(use_entity)) {
                pre = "Use";
            }

            if (pre) {
                auto name = *type_man.get_instance_data(use_entity).name;
                sprintf(buf2, "%s %s the %s", key, pre, name);
                w = 0;
                h = 0;
                text->measure(buf2, &w, &h);
                add_text_with_outline(buf2, -w / 2, -200);
            }
        }


        /* debug text */
        if (draw_debug_text) {
            /* Atmo status */
            glm::ivec3 eye_block = get_coord_containing(pl.eye);

            topo_info *t = topo_find(ship->get_topo_info(eye_block));
            topo_info *outside = topo_find(&ship->outside_topo_info);
            zone_info *z = ship->get_zone_info(t);
            float pressure = z ? (z->air_amount / t->size) : 0.0f;

            if (t != outside) {
                sprintf(buf2, "[INSIDE %p %d %.1f atmo %.2f]", t, t->size, pressure, pl.thing);
            }
            else {
                sprintf(buf2, "[OUTSIDE %p %d %.1f atmo %.2f]", t, t->size, pressure, pl.thing);
            }

            w = 0; h = 0;
            text->measure(buf2, &w, &h);
            add_text_with_outline(buf2, -w/2, -100);

            w = 0; h = 0;
            sprintf(buf2, "full: %d fast-unify: %d fast-nosplit: %d false-split: %d",
                    ship->num_full_rebuilds,
                    ship->num_fast_unifys,
                    ship->num_fast_nosplits,
                    ship->num_false_splits);
            text->measure(buf2, &w, &h);
            add_text_with_outline(buf2, -w/2, -150);
        }

        for (unsigned i = 0; i < tools.size(); i++) {
            ui_sprites->add(pl.active_tool_slot == i ? &lit_ui_slot_sprite : &unlit_ui_slot_sprite,
                            (i - tools.size() / 2.0f) * 34, -220);
        }
    }

    void update(float dt) override {
        if (window_has_focus() && SDL_GetRelativeMouseMode() == SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }

        if (!window_has_focus() && SDL_GetRelativeMouseMode() != SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }

        auto *t = tools[pl.active_tool_slot];

        if (t) {
            t->pre_use(&pl);

            /* tool use */
            if (pl.use_tool) {
                t->use();
                pl.ui_dirty = true;

                assert(ship->validate());
            }

            if (pl.alt_use_tool) {
                t->alt_use();
                pl.ui_dirty = true;
            }

            if (pl.long_use_tool) {
                t->long_use();
                pl.ui_dirty = true;
            }

            if (pl.long_alt_use_tool) {
                t->long_alt_use();
                pl.ui_dirty = true;
            }

            if (pl.cycle_mode) {
                t->cycle_mode();
                pl.ui_dirty = true;
            }
        }

        /* can only interact with entities which have
         * the switch component
         */
        auto &switch_man = component_system_man.managers.switch_component_man;
        auto &surf_man = component_system_man.managers.surface_attachment_component_man;
        raycast_info_world rc_ent;
        phys_raycast_world(pl.eye, pl.eye + 2.f * pl.dir,
                           phy->rb_controller.get(), phy->dynamicsWorld.get(), &rc_ent);

        auto old_entity = use_entity;
        use_entity = rc_ent.entity;
        if (rc_ent.hit && c_entity::is_valid(rc_ent.entity)) {
            // if entity is not bolted down, remove from world
            // otherwise, interact
            if ((!surf_man.exists(rc_ent.entity) ||
                 !*(surf_man.get_instance_data(rc_ent.entity)).attached) && pl.use) {
                destroy_entity(rc_ent.entity);
                use_entity.id = 0;
            } else if (switch_man.exists(rc_ent.entity)) {
                if (pl.use && c_entity::is_valid(rc_ent.entity)) {
                    use_action_on_entity(ship, rc_ent.entity);
                }
            }
        }
        if (old_entity != use_entity) {
            pl.ui_dirty = true;
        }
    }

    void render(frame_data *frame) override {
    }

    void set_slot(unsigned slot) {
        /* note: all the number keys are bound, but we may not have 10 toolbelt slots.
         * just drop bogus slot requests on the floor.
         */
        if (slot < tools.size() && slot != pl.active_tool_slot) {
            auto *old_tool = tools[pl.active_tool_slot];
            if (old_tool) {
                old_tool->unselect();
            }

            tools[slot]->select();

            pl.active_tool_slot = slot;
            pl.ui_dirty = true;
        }
    }

    void cycle_slot(int d) {
        unsigned num_tools = unsigned(tools.size());
        unsigned int cur_slot = pl.active_tool_slot;
        cur_slot = (cur_slot + num_tools + d) % num_tools;

        set_slot(cur_slot);
    }

    void handle_input() override {
        /* look */
        auto look_x     = get_input(action_look_x)->value;
        auto look_y     = get_input(action_look_y)->value;

        /* movement */
        auto move_x     = get_input(action_right)->active - get_input(action_left)->active;
        auto move_y     = get_input(action_forward)->active - get_input(action_back)->active;
        auto move_z     = get_input(action_up)->active - get_input(action_down)->active;
        auto roll       = get_input(action_roll_right)->value - get_input(action_roll_left)->value;

        /* crouch */
        auto crouch     = get_input(action_crouch)->active;
        auto crouch_end = get_input(action_crouch)->just_inactive;

        /* momentary */
        auto jump       = get_input(action_jump)->just_active;
        auto reset      = get_input(action_reset)->just_active;
        auto use        = get_input(action_use)->just_active;
        auto cycle_mode = get_input(action_cycle_mode)->just_active;
        auto slot0      = get_input(action_slot1)->just_active;
        auto slot1      = get_input(action_slot2)->just_active;
        auto slot2      = get_input(action_slot3)->just_active;
        auto slot3      = get_input(action_slot4)->just_active;
        auto slot4      = get_input(action_slot5)->just_active;
        auto slot5      = get_input(action_slot6)->just_active;
        auto slot6      = get_input(action_slot7)->just_active;
        auto slot7      = get_input(action_slot8)->just_active;
        auto slot8      = get_input(action_slot9)->just_active;
        auto slot9      = get_input(action_slot0)->just_active;
        auto next_tool  = get_input(action_tool_next)->just_active;
        auto prev_tool  = get_input(action_tool_prev)->just_active;

        auto input_use_tool     = get_input(action_use_tool);
        auto use_tool           = input_use_tool->just_pressed;
        auto long_use_tool      = input_use_tool->held;
        auto input_alt_use_tool = get_input(action_alt_use_tool);
        auto alt_use_tool       = input_alt_use_tool->just_pressed;
        auto long_alt_use_tool  = input_alt_use_tool->held;

        /* persistent */

        float mouse_invert = game_settings.input.mouse_invert;

        pl.move = { (float)move_x, (float)move_y, (float)move_z };

        pl.roll = glm::lerp(pl.roll, roll, 0.05f);
        pl.jump              = jump;
        pl.crouch            = crouch;
        pl.reset             = reset;
        pl.crouch_end        = crouch_end;
        pl.use               = use;
        pl.cycle_mode        = cycle_mode;
        pl.use_tool          = use_tool;
        pl.alt_use_tool      = alt_use_tool;
        pl.long_use_tool     = long_use_tool;
        pl.long_alt_use_tool = long_alt_use_tool;

        auto pitch = glm::angleAxis(game_settings.input.mouse_y_sensitivity * look_y * mouse_invert, glm::vec3{ 1.f,0.f,0.f });
        auto yaw = glm::angleAxis(game_settings.input.mouse_x_sensitivity * look_x, glm::vec3{ 0.f,1.f,0.f });
        auto roll_quat = glm::angleAxis(-.03f * pl.roll, glm::vec3{0.f, 0.f, 1.f});
        pl.rot = pl.rot * pitch * yaw * roll_quat;

        // blech. Tool gets used below, then fire projectile gets hit here
        if (pl.fire_projectile) {
            auto below_eye = glm::vec3(pl.eye.x, pl.eye.y, pl.eye.z - 0.1);
            proj_man.spawn(below_eye, pl.dir);
            pl.fire_projectile = false;
        }

        if (next_tool) {
            cycle_slot(1);
        }
        if (prev_tool) {
            cycle_slot(-1);
        }

        if (slot1) set_slot(1);
        if (slot2) set_slot(2);
        if (slot3) set_slot(3);
        if (slot4) set_slot(4);
        if (slot5) set_slot(5);
        if (slot6) set_slot(6);
        if (slot7) set_slot(7);
        if (slot8) set_slot(8);
        if (slot9) set_slot(9);
        if (slot0) set_slot(0);

        /* limit to unit vector */
        float len = glm::length(pl.move);
        if (len > 0.0f)
            pl.move = pl.move / len;

        if (get_input(action_menu)->just_active) {
            set_next_game_state(create_menu_state());
        }
    }
};

game_state *game_state::create_play_state() { return new play_state; }
