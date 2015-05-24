#include "config.h"

#include <libconfig.h>
#include "libconfig_shim.h"


void
configureBindings(std::unordered_map<en_action, action, std::hash<int>> &en_actions) {
    const char *keysPath = "configs/keys.cfg";
    config_t cfg;
    config_setting_t *binds;

    config_init(&cfg);

    if (!config_read_file(&cfg, keysPath))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg), keysPath);
        config_destroy(&cfg);
    }

    binds = config_lookup(&cfg, "binds");

    if (binds != NULL) {
        /* http://www.hyperrealm.com/libconfig/libconfig_manual.html
        * states
        *  > int config_setting_length (const config_setting_t * setting)
        *  > This function returns the number of settings in a group,
        *  > or the number of elements in a list or array.
        *  > For other types of settings, it returns 0.
        *
        * so this can only ever be positive, despite the return type being int
        */
        unsigned int count = config_setting_length(binds);

        for (unsigned int i = 0; i < count; ++i) {
            config_setting_t *bind, *inputs_keyboard, *inputs_mouse;
            bind = config_setting_get_elem(binds, i);

            const char **inputs_keyboard_names = NULL;
            const char **inputs_mouse_names = NULL;
            const char *action_name = NULL;
            unsigned int inputs_keyboard_count = 0;
            unsigned int inputs_mouse_count = 0;

            config_setting_lookup_string(bind, "action", &action_name);
            if (!action_name) {
                continue;
            }

            inputs_keyboard = config_setting_lookup(bind, "inputs_keyboard");
            inputs_mouse = config_setting_lookup(bind, "inputs_mouse");

            /* config_setting_length will only ever be 0 or positive according
            * to the docs
            * */
            if (inputs_keyboard) {
                inputs_keyboard_count = config_setting_length(inputs_keyboard);
                inputs_keyboard_names = (const char**)malloc(sizeof(char*) * inputs_keyboard_count);

                for (unsigned int input_index = 0; input_index < inputs_keyboard_count; ++input_index) {
                    config_setting_t *input = config_setting_get_elem(inputs_keyboard, input_index);
                    inputs_keyboard_names[input_index] = config_setting_get_string(input);
                }
            }
            if (inputs_mouse) {
                inputs_mouse_count = config_setting_length(inputs_mouse);
                inputs_mouse_names = (const char**)malloc(sizeof(char*) * inputs_mouse_count);

                for (unsigned int input_index = 0; input_index < inputs_mouse_count; ++input_index) {
                    config_setting_t *input = config_setting_get_elem(inputs_mouse, input_index);
                    inputs_mouse_names[input_index] = config_setting_get_string(input);
                }
            }

            unsigned int input_index = 0;
            en_action i_action = lookup_action(action_name);
            en_actions[i_action] = action(i_action);

            for (input_index = 0; input_index < inputs_keyboard_count; ++input_index) {
                en_input input = lookup_input(inputs_keyboard_names[input_index]);
                en_actions[i_action].bind_key(input);
            }

            for (input_index = 0; input_index < inputs_mouse_count; ++input_index) {
                en_input input = lookup_input(inputs_mouse_names[input_index]);
                en_actions[i_action].bind_mouse(input);
            }

            if (inputs_keyboard_names)
                free(inputs_keyboard_names);

            if (inputs_mouse_names)
                free(inputs_mouse_names);
        }
    }
}