//
// Created by caleb on 3/17/18.
//
#pragma once

#include "render_data.h"
#include "component/c_entity.h"

struct ship_space;

class game_state {

public:
    virtual ~game_state() = default;

    virtual void handle_input() = 0;

    virtual void update(float dt) = 0;

    virtual void render(frame_data *frame) = 0;

    virtual void rebuild_ui() = 0;

    static game_state *create_play_state();
    static game_state *create_menu_state();
    static game_state *create_customize_entity_comms_filter_state(c_entity e);
    static game_state *create_customize_entity_comms_output_state(c_entity e);
    static game_state *create_customize_entity_comms_inspection_state(ship_space *ship, c_entity e);
};


