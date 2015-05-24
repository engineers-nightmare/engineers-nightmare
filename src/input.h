#pragma once

#include <SDL.h>

#include <vector>


enum en_action {
    action_invalid = -1,
    action_left,
    action_right,
    action_forward,
    action_back,
    action_jump,
    action_use,
    action_menu,
    action_reset,
    action_crouch,
    action_gravity,
    action_tool_next,
    action_tool_prev,
    action_use_tool,
    action_slot1,
    action_slot2,
    action_slot3,
    action_slot4,
    action_slot5,
    action_slot6,
    action_slot7,
    action_slot8,
    action_slot9,

    num_actions,
};

struct action_lookup_t {
    const char* name;
    en_action action;
};

static const action_lookup_t action_lookup_table[] = {
    { "action_left",      action_left },
    { "action_right",     action_right },
    { "action_forward",   action_forward },
    { "action_back",      action_back },
    { "action_jump",      action_jump },
    { "action_use",       action_use },
    { "action_menu",      action_menu },
    { "action_reset",     action_reset },
    { "action_crouch",    action_crouch },
    { "action_gravity",   action_gravity },
    { "action_tool_next", action_tool_next },
    { "action_tool_prev", action_tool_prev },
    { "action_use_tool",  action_use_tool },
    { "action_slot1",     action_slot1 },
    { "action_slot2",     action_slot2 },
    { "action_slot3",     action_slot3 },
    { "action_slot4",     action_slot4 },
    { "action_slot5",     action_slot5 },
    { "action_slot6",     action_slot6 },
    { "action_slot7",     action_slot7 },
    { "action_slot8",     action_slot8 },
    { "action_slot9",     action_slot9 },
};

/* fairly ugly. non-keyboard inputs go at bottom
keyboard entries trimmed down from SDL list */
enum en_input {
    input_invalid          = -1,
    input_a                = SDL_SCANCODE_A,
    input_b                = SDL_SCANCODE_B,
    input_c                = SDL_SCANCODE_C,
    input_d                = SDL_SCANCODE_D,
    input_e                = SDL_SCANCODE_E,
    input_f                = SDL_SCANCODE_F,
    input_g                = SDL_SCANCODE_G,
    input_h                = SDL_SCANCODE_H,
    input_i                = SDL_SCANCODE_I,
    input_j                = SDL_SCANCODE_J,
    input_k                = SDL_SCANCODE_K,
    input_l                = SDL_SCANCODE_L,
    input_m                = SDL_SCANCODE_M,
    input_n                = SDL_SCANCODE_N,
    input_o                = SDL_SCANCODE_O,
    input_p                = SDL_SCANCODE_P,
    input_q                = SDL_SCANCODE_Q,
    input_r                = SDL_SCANCODE_R,
    input_s                = SDL_SCANCODE_S,
    input_t                = SDL_SCANCODE_T,
    input_u                = SDL_SCANCODE_U,
    input_v                = SDL_SCANCODE_V,
    input_w                = SDL_SCANCODE_W,
    input_x                = SDL_SCANCODE_X,
    input_y                = SDL_SCANCODE_Y,
    input_z                = SDL_SCANCODE_Z,
    input_1                = SDL_SCANCODE_1,
    input_2                = SDL_SCANCODE_2,
    input_3                = SDL_SCANCODE_3,
    input_4                = SDL_SCANCODE_4,
    input_5                = SDL_SCANCODE_5,
    input_6                = SDL_SCANCODE_6,
    input_7                = SDL_SCANCODE_7,
    input_8                = SDL_SCANCODE_8,
    input_9                = SDL_SCANCODE_9,
    input_0                = SDL_SCANCODE_0,
    input_return           = SDL_SCANCODE_RETURN,
    input_escape           = SDL_SCANCODE_ESCAPE,
    input_backspace        = SDL_SCANCODE_BACKSPACE,
    input_tab              = SDL_SCANCODE_TAB,
    input_space            = SDL_SCANCODE_SPACE,
    input_minus            = SDL_SCANCODE_MINUS,
    input_equals           = SDL_SCANCODE_EQUALS,
    input_leftbracket      = SDL_SCANCODE_LEFTBRACKET,
    input_rightbracket     = SDL_SCANCODE_RIGHTBRACKET,
    input_backslash        = SDL_SCANCODE_BACKSLASH,
    input_nonushash        = SDL_SCANCODE_NONUSHASH,
    input_semicolon        = SDL_SCANCODE_SEMICOLON,
    input_apostrophe       = SDL_SCANCODE_APOSTROPHE,
    input_grave            = SDL_SCANCODE_GRAVE,
    input_comma            = SDL_SCANCODE_COMMA,
    input_period           = SDL_SCANCODE_PERIOD,
    input_slash            = SDL_SCANCODE_SLASH,
    input_capslock         = SDL_SCANCODE_CAPSLOCK,
    input_f1               = SDL_SCANCODE_F1,
    input_f2               = SDL_SCANCODE_F2,
    input_f3               = SDL_SCANCODE_F3,
    input_f4               = SDL_SCANCODE_F4,
    input_f5               = SDL_SCANCODE_F5,
    input_f6               = SDL_SCANCODE_F6,
    input_f7               = SDL_SCANCODE_F7,
    input_f8               = SDL_SCANCODE_F8,
    input_f9               = SDL_SCANCODE_F9,
    input_f10              = SDL_SCANCODE_F10,
    input_f11              = SDL_SCANCODE_F11,
    input_f12              = SDL_SCANCODE_F12,
    input_printscreen      = SDL_SCANCODE_PRINTSCREEN,
    input_scrolllock       = SDL_SCANCODE_SCROLLLOCK,
    input_pause            = SDL_SCANCODE_PAUSE,
    input_insert           = SDL_SCANCODE_INSERT,
    input_home             = SDL_SCANCODE_HOME,
    input_pageup           = SDL_SCANCODE_PAGEUP,
    input_delete           = SDL_SCANCODE_DELETE,
    input_end              = SDL_SCANCODE_END,
    input_pagedown         = SDL_SCANCODE_PAGEDOWN,
    input_right            = SDL_SCANCODE_RIGHT,
    input_left             = SDL_SCANCODE_LEFT,
    input_down             = SDL_SCANCODE_DOWN,
    input_up               = SDL_SCANCODE_UP,
    input_numlockclear     = SDL_SCANCODE_NUMLOCKCLEAR,
    input_kp_divide        = SDL_SCANCODE_KP_DIVIDE,
    input_kp_multiply      = SDL_SCANCODE_KP_MULTIPLY,
    input_kp_minus         = SDL_SCANCODE_KP_MINUS,
    input_kp_plus          = SDL_SCANCODE_KP_PLUS,
    input_kp_enter         = SDL_SCANCODE_KP_ENTER,
    input_kp_1             = SDL_SCANCODE_KP_1,
    input_kp_2             = SDL_SCANCODE_KP_2,
    input_kp_3             = SDL_SCANCODE_KP_3,
    input_kp_4             = SDL_SCANCODE_KP_4,
    input_kp_5             = SDL_SCANCODE_KP_5,
    input_kp_6             = SDL_SCANCODE_KP_6,
    input_kp_7             = SDL_SCANCODE_KP_7,
    input_kp_8             = SDL_SCANCODE_KP_8,
    input_kp_9             = SDL_SCANCODE_KP_9,
    input_kp_0             = SDL_SCANCODE_KP_0,
    input_kp_period        = SDL_SCANCODE_KP_PERIOD,
    input_nonusbackslash   = SDL_SCANCODE_NONUSBACKSLASH,
    input_lctrl            = SDL_SCANCODE_LCTRL,
    input_lshift           = SDL_SCANCODE_LSHIFT,
    input_lalt             = SDL_SCANCODE_LALT,
    input_lgui             = SDL_SCANCODE_LGUI,
    input_rctrl            = SDL_SCANCODE_RCTRL,
    input_rshift           = SDL_SCANCODE_RSHIFT,
    input_ralt             = SDL_SCANCODE_RALT,
    input_rgui             = SDL_SCANCODE_RGUI,

    /* end of keyboard */
    input_eok              = SDL_NUM_SCANCODES,

    input_mouse_left       = input_eok + SDL_BUTTON_LEFT,
    input_mouse_middle     = input_eok + SDL_BUTTON_MIDDLE,
    input_mouse_right      = input_eok + SDL_BUTTON_RIGHT,
    input_mouse_thumb1     = input_eok + SDL_BUTTON_X1,
    input_mouse_thumb2     = input_eok + SDL_BUTTON_X2,
    input_mouse_wheeldown,
    input_mouse_wheelup,

    /* end of mouse */
    input_eom,

    input_num_inputs,
};

struct input_lookup_t {
    const char* name; en_input action;
};

static const input_lookup_t input_lookup_table[] = {
    { "input_a",               input_a },
    { "input_b",               input_b },
    { "input_c",               input_c },
    { "input_d",               input_d },
    { "input_e",               input_e },
    { "input_f",               input_f },
    { "input_g",               input_g },
    { "input_h",               input_h },
    { "input_i",               input_i },
    { "input_j",               input_j },
    { "input_k",               input_k },
    { "input_l",               input_l },
    { "input_m",               input_m },
    { "input_n",               input_n },
    { "input_o",               input_o },
    { "input_p",               input_p },
    { "input_q",               input_q },
    { "input_r",               input_r },
    { "input_s",               input_s },
    { "input_t",               input_t },
    { "input_u",               input_u },
    { "input_v",               input_v },
    { "input_w",               input_w },
    { "input_x",               input_x },
    { "input_y",               input_y },
    { "input_z",               input_z },
    { "input_1",               input_1 },
    { "input_2",               input_2 },
    { "input_3",               input_3 },
    { "input_4",               input_4 },
    { "input_5",               input_5 },
    { "input_6",               input_6 },
    { "input_7",               input_7 },
    { "input_8",               input_8 },
    { "input_9",               input_9 },
    { "input_0",               input_0 },
    { "input_return",          input_return },
    { "input_escape",          input_escape },
    { "input_backspace",       input_backspace },
    { "input_tab",             input_tab },
    { "input_space",           input_space },
    { "input_minus",           input_minus },
    { "input_equals",          input_equals },
    { "input_leftbracket",     input_leftbracket },
    { "input_rightbracket",    input_rightbracket },
    { "input_backslash",       input_backslash },
    { "input_nonushash",       input_nonushash },
    { "input_semicolon",       input_semicolon },
    { "input_apostrophe",      input_apostrophe },
    { "input_grave",           input_grave },
    { "input_comma",           input_comma },
    { "input_period",          input_period },
    { "input_slash",           input_slash },
    { "input_capslock",        input_capslock },
    { "input_f1",              input_f1 },
    { "input_f2",              input_f2 },
    { "input_f3",              input_f3 },
    { "input_f4",              input_f4 },
    { "input_f5",              input_f5 },
    { "input_f6",              input_f6 },
    { "input_f7",              input_f7 },
    { "input_f8",              input_f8 },
    { "input_f9",              input_f9 },
    { "input_f10",             input_f10 },
    { "input_f11",             input_f11 },
    { "input_f12",             input_f12 },
    { "input_printscreen",     input_printscreen },
    { "input_scrolllock",      input_scrolllock },
    { "input_pause",           input_pause },
    { "input_insert",          input_insert },
    { "input_home",            input_home },
    { "input_pageup",          input_pageup },
    { "input_delete",          input_delete },
    { "input_end",             input_end },
    { "input_pagedown",        input_pagedown },
    { "input_right",           input_right },
    { "input_left",            input_left },
    { "input_down",            input_down },
    { "input_up",              input_up },
    { "input_numlockclear",    input_numlockclear },
    { "input_kp_divide",       input_kp_divide },
    { "input_kp_multiply",     input_kp_multiply },
    { "input_kp_minus",        input_kp_minus },
    { "input_kp_plus",         input_kp_plus },
    { "input_kp_enter",        input_kp_enter },
    { "input_kp_1",            input_kp_1 },
    { "input_kp_2",            input_kp_2 },
    { "input_kp_3",            input_kp_3 },
    { "input_kp_4",            input_kp_4 },
    { "input_kp_5",            input_kp_5 },
    { "input_kp_6",            input_kp_6 },
    { "input_kp_7",            input_kp_7 },
    { "input_kp_8",            input_kp_8 },
    { "input_kp_9",            input_kp_9 },
    { "input_kp_0",            input_kp_0 },
    { "input_kp_period",       input_kp_period },
    { "input_nonusbackslash",  input_nonusbackslash },
    { "input_lctrl",           input_lctrl },
    { "input_lshift",          input_lshift },
    { "input_lalt",            input_lalt },
    { "input_lgui",            input_lgui },
    { "input_rctrl",           input_rctrl },
    { "input_rshift",          input_rshift },
    { "input_ralt",            input_ralt },
    { "input_rgui",            input_rgui },
    { "input_mouse_left",      input_mouse_left },
    { "input_mouse_middle",    input_mouse_middle },
    { "input_mouse_right",     input_mouse_right },
    { "input_mouse_thumb1",    input_mouse_thumb1 },
    { "input_mouse_thumb2",    input_mouse_thumb2 },
    { "input_mouse_wheeldown", input_mouse_wheeldown },
    { "input_mouse_wheelup",   input_mouse_wheelup },
};

/* The lookup_* functions aren't optimized. They just do a linear walk
* through the lookup tables. This is probably fine as the tables shouldn't
* get _too_ large, nor are these likely to be called very frequently.
*/
en_action lookup_action(const char *lookup);

/* This is probably only useful for populating config files */
const char*  lookup_input_action(en_action lookup);

en_input lookup_input(const char *lookup);

/* This is probably only useful for populating config files */
const char* lookup_input(en_input lookup);

struct binding {
    std::vector<en_input> keyboard_inputs;
    std::vector<en_input> mouse_inputs;

    void bind_key(en_input key) {
        if (key != input_eok) {
            keyboard_inputs.push_back(key);
        }
    }

    void bind_mouse(en_input mouse) {
        if (mouse != input_eom) {
            mouse_inputs.push_back(mouse);
        }
    }
};

struct action {
    en_action input;
    binding binds;

    bool active = false;        /* is action currently active */
    bool just_active = false;   /* did action go active this frame */
    bool just_inactive = false; /* did action go inactive this frame */
    Uint32 last_active = 0;     /* time of last activation */
    Uint32 current_active = 0;  /* duration of current activation */

    action()
    {
    }

    action(en_action a) :
        input(a)
    {
    }

    action(en_action a, binding b) :
        input(a), binds(b)
    {
    }

    void bind_key(en_input key) {
        binds.bind_key(key);
    }

    void bind_mouse(en_input mouse) {
        binds.bind_mouse(mouse);
    }
};
