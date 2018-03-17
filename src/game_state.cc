//
// Created by caleb on 3/17/18.
//

#include "game_state.h"
#include "game_state/play_state.h"
#include "game_state/menu_state.h"
#include "game_state/customize_entity_state.h"

game_state *game_state::create_play_state() { return new play_state; }
game_state *game_state::create_menu_state() { return new menu_state; }
game_state *game_state::create_customize_entity_state() { return new customize_entity_state; }
