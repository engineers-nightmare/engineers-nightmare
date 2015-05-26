#include "input.h"

/* The lookup_* functions aren't optimized. They just do a linear walk
* through the lookup tables. This is probably fine as the tables shouldn't
* get _too_ large, nor are these likely to be called very frequently.
*/
en_action lookup_action(const char *lookup) {
    for (const action_lookup_t *input = action_lookup_table; input->name != NULL; ++input) {
        if (strcmp(input->name, lookup) == 0) {
            return input->action;
        }
    }
    return action_invalid;
}

/* This is probably only useful for populating config files */
const char*  lookup_input_action(en_action lookup) {
    for (const action_lookup_t *input = action_lookup_table; input->name != NULL; ++input) {
        if (input->action == lookup) {
            return input->name;
        }
    }
    return NULL;
}

en_input lookup_input(const char *lookup) {
    for (const input_lookup_t *input = input_lookup_table; input->name != NULL; ++input) {
        if (strcmp(input->name, lookup) == 0) {
            return input->action;
        }
    }
    return input_invalid;
}

/* This is probably only useful for populating config files */
const char* lookup_input(en_input lookup) {
    for (const input_lookup_t *input = input_lookup_table; input->name != NULL; ++input) {
        if (input->action == lookup) {
            return input->name;
        }
    }
    return NULL;
}

void
set_inputs(unsigned char const * keys,
const unsigned int mouse_buttons[],
std::unordered_map<en_action, action, std::hash<int>> &actions) {
    auto now = SDL_GetTicks();

    for (auto &actionPair : actions) {
        bool active = false;
        auto action = &actionPair.second;
        auto binds = &action->binds;

        for (auto &key : binds->keyboard_inputs) {
            if (keys[key]) {
                active = true;
            }
        }

        for (auto &mouse : binds->mouse_inputs) {
            if (mouse_buttons[EN_BUTTON(mouse)]) {
                active |= true;
            }
        }

        /* currently active */
        if (action->active) {
            /* still active */
            if (active) {
                /* set everything to ensure full state maintained */
                action->value = 1.f;
                action->active = true;
                action->just_active = false;
                action->just_inactive = false;
                action->last_active = action->last_active;
                action->current_active = now - action->last_active;
            }
            /* just deactivated */
            else {
                action->value = 0.f;
                action->active = false;
                action->just_active = false;
                action->just_inactive = true;
                action->last_active = action->last_active;
                action->current_active = 0;
            }
        }
        /* not currently active */
        else {
            /* just active */
            if (active) {
                action->value = 1.f;
                action->active = true;
                action->just_active = true;
                action->just_inactive = false;
                action->last_active = now;
                action->current_active = 0;
            }
            /* still not active */
            else {
                action->value = 0.f;
                action->active = false;
                action->just_active = false;
                action->just_inactive = false;
                action->last_active = action->last_active;
                action->current_active = 0;
            }
        }
    }
}
