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
