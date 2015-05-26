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
            config_setting_t *bind, *inputs;
            bind = config_setting_get_elem(binds, i);

            const char **inputs_names = NULL;
            const char *action_name = NULL;
            unsigned int inputs_count = 0;

            config_setting_lookup_string(bind, "action", &action_name);
            if (!action_name) {
                continue;
            }

            inputs = config_setting_lookup(bind, "inputs");

            /* config_setting_length will only ever be 0 or positive according
            * to the docs
            * */
            if (inputs) {
                inputs_count = config_setting_length(inputs);
                inputs_names = (const char**)malloc(sizeof(char*) * inputs_count);

                for (unsigned int input_index = 0; input_index < inputs_count; ++input_index) {
                    config_setting_t *input = config_setting_get_elem(inputs, input_index);
                    inputs_names[input_index] = config_setting_get_string(input);
                }
            }

            unsigned int input_index = 0;
            en_action i_action = lookup_action(action_name);
            en_actions[i_action] = action(i_action);

            for (input_index = 0; input_index < inputs_count; ++input_index) {
                en_input input = lookup_input(inputs_names[input_index]);
                en_actions[i_action].bind(input);
            }

            if (inputs)
                free(inputs);
        }
    }
}