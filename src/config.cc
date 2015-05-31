#include "config.h"

#include <libconfig.h>
#include "libconfig_shim.h"

#define KEYS_CONFIG_PATH "configs/keys.cfg"
#define VIDEO_CONFIG_PATH "configs/video.cfg"
#define INPUT_CONFIG_PATH "configs/input.cfg"

void
configure_settings(settings &settings) {
    configure_video_settings(settings.video);
    configure_input_settings(settings.input);
    configure_bindings(settings.bindings);
}

void
configure_bindings(std::unordered_map<en_action, action, std::hash<int>> &en_actions) {
    config_t cfg;
    config_setting_t *binds_config_setting;

    config_init(&cfg);

    if (!config_read_file(&cfg, KEYS_CONFIG_PATH))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg), KEYS_CONFIG_PATH);
        config_destroy(&cfg);

        return;
    }

    binds_config_setting = config_lookup(&cfg, "binds");

    if (binds_config_setting != NULL) {
        /* http://www.hyperrealm.com/libconfig/libconfig_manual.html
        * states
        *  > int config_setting_length (const config_setting_t * setting)
        *  > This function returns the number of settings in a group,
        *  > or the number of elements in a list or array.
        *  > For other types of settings, it returns 0.
        *
        * so this can only ever be positive, despite the return type being int
        */
        unsigned int count = config_setting_length(binds_config_setting);

        for (unsigned int i = 0; i < count; ++i) {
            config_setting_t *bind, *inputs;
            bind = config_setting_get_elem(binds_config_setting, i);

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

void
configure_video_settings(video_settings &video_settings) {
    /* nothing configured yet */
}

void
configure_input_settings(input_settings &input_settings) {
    config_t cfg;
    config_setting_t *input_config_setting;

    config_init(&cfg);

    if (!config_read_file(&cfg, INPUT_CONFIG_PATH))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg), INPUT_CONFIG_PATH);
        config_destroy(&cfg);

        return;
    }

    input_config_setting = config_lookup(&cfg, "input");

    if (input_config_setting != NULL) {
        int mouse_invert = 0;
        double mouse_x_sensitivity = 0.0;
        double mouse_y_sensitivity = 0.0;

        /* mouse_invert */
        int success = config_setting_lookup_bool(
            input_config_setting, "mouse_invert", &mouse_invert);

        if (success == CONFIG_TRUE) {
            input_settings.mouse_invert = (mouse_invert != 0);
        }

        /* mouse_x_sensitivity */
        success = config_setting_lookup_float(
            input_config_setting, "mouse_x_sensitivity", &mouse_x_sensitivity);

        if (success == CONFIG_TRUE) {
            input_settings.mouse_x_sensitivity = (float)mouse_x_sensitivity;
        }

        /* mouse_y_sensitivity */
        success = config_setting_lookup_float(
            input_config_setting, "mouse_y_sensitivity", &mouse_y_sensitivity);

        if (success == CONFIG_TRUE) {
            input_settings.mouse_y_sensitivity = (float)mouse_x_sensitivity;
        }
    }
}
