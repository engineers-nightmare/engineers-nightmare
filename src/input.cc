#include "input.h"

// ms duration under which a momentary input is registered
constexpr unsigned int pressed_input_duration = 200;

enum input_type {
    input_type_invalid,
    input_type_keyboard,
    input_type_mouse_button,
    input_type_mouse_axis,
};

/* dependent on proper grouping in en_input */
input_type
get_input_type(en_input input) {
    input_type type = input_type_invalid;

    if (input_keyboard_keys_start <= input && input <= input_keyboard_keys_end) {
        type = input_type_keyboard;
    }
    else if (input_mouse_buttons_start <= input && input <= input_mouse_buttons_end) {
        type = input_type_mouse_button;
    }
    else if (input_mouse_axes_start <= input && input <= input_mouse_axes_end) {
        type = input_type_mouse_axis;
    }

    return type;
}

/* The lookup_* functions aren't optimized. They just do a linear walk
* through the lookup tables. This is probably fine as the tables shouldn't
* get _too_ large, nor are these likely to be called very frequently.
*/
en_action
lookup_action(const char *lookup) {
    for (const action_lookup_t *input = action_lookup_table; input->name != nullptr; ++input) {
        if (strcmp(input->name, lookup) == 0) {
            return input->action;
        }
    }
    return action_invalid;
}

/* This is probably only useful for populating config files */
const char*
lookup_input_action(en_action lookup) {
    for (const action_lookup_t *input = action_lookup_table; input->name != nullptr; ++input) {
        if (input->action == lookup) {
            return input->name;
        }
    }
    return nullptr;
}

en_input
lookup_input(const char *lookup) {
    for (const input_lookup_t *input = input_lookup_table; input->name != nullptr; ++input) {
        if (strcmp(input->name, lookup) == 0) {
            return input->action;
        }
    }
    return input_invalid;
}

/* This is probably only useful for populating config files */
const char*
lookup_input(en_input lookup) {
    for (const input_lookup_t *input = input_lookup_table; input->name != nullptr; ++input) {
        if (input->action == lookup) {
            return input->name;
        }
    }
    return nullptr;
}

/* This is used for getting string representation of input key */
/* for example input_a => "[ A ]" */
const char*
lookup_key(en_input lookup) {
    for (const key_lookup_t *key = key_lookup_table; key->name != nullptr; ++key) {
        if (key->action == lookup) {
            return key->name;
        }
    }
    return nullptr;
}

void
set_inputs(unsigned char const * keys,
const unsigned int mouse_buttons[],
const int mouse_axes[],
std::unordered_map<en_action, action, std::hash<int>> &actions) {
    auto now = SDL_GetTicks();

    for (auto &actionPair : actions) {
        bool active = false;
        float axis_value = 0.f;
        auto action = &actionPair.second;
        auto binds = &action->binds;

        for (auto &input : binds->inputs) {
            switch (get_input_type(input))
            {
            default:
            case input_type_invalid:
                break;
            case input_type_keyboard:
                if (keys[input]) {
                    active = true;
                    axis_value = 1.f;
                }
                break;
            case input_type_mouse_button:
                if (mouse_buttons[EN_MOUSE_BUTTON(input)]) {
                    active |= true;
                    axis_value = 1.f;
                }
                break;
            case input_type_mouse_axis:
                if (mouse_axes[EN_MOUSE_AXIS(input)]) {
                    active |= true;
                    axis_value = (float)mouse_axes[EN_MOUSE_AXIS(input)];
                }
                break;
            }
        }

        /* currently active */
        if (action->active) {
            /* still active */
            if (active) {
                /* set everything to ensure full state maintained */
                action->value = axis_value;
                action->active = true;
                action->just_active = false;
                action->just_inactive = false;
                action->just_pressed = false;
                action->last_active = action->last_active;
                action->current_active = now - action->last_active;
                if (action->current_active > pressed_input_duration) {
                    action->held = true;
                }
            }
            /* just deactivated */
            else {
                action->value = 0.f;
                action->active = false;
                action->just_active = false;
                action->just_inactive = true;
                action->held = false;
                action->last_active = action->last_active;
                if (action->current_active <= pressed_input_duration) {
                    action->just_pressed = true;
                }
                action->current_active = 0;
            }
        }
        /* not currently active */
        else {
            /* just active */
            if (active) {
                action->value = axis_value;
                action->active = true;
                action->just_active = true;
                action->just_inactive = false;
                action->just_pressed = false;
                action->held = false;
                action->last_active = now;
                action->current_active = 0;
            }
            /* still not active */
            else {
                action->value = 0.f;
                action->active = false;
                action->just_active = false;
                action->just_inactive = false;
                action->just_pressed = false;
                action->held = false;
                action->last_active = action->last_active;
                action->current_active = 0;
            }
        }
    }
}
