#include "config.h"

#include <libconfig.h>

#include <utility>
#include "libconfig_shim.h"

#define BASE_CONFIG_PATH "configs/base/"
#define USER_CONFIG_PATH "configs/user/"
#define KEYS_CONFIG  "keys.cfg"
#define VIDEO_CONFIG "video.cfg"
#define INPUT_CONFIG "input.cfg"

#define BASE_KEYS_CONFIG_PATH  BASE_CONFIG_PATH KEYS_CONFIG 
#define BASE_VIDEO_CONFIG_PATH BASE_CONFIG_PATH VIDEO_CONFIG
#define BASE_INPUT_CONFIG_PATH BASE_CONFIG_PATH INPUT_CONFIG

#define USER_KEYS_CONFIG_PATH  USER_CONFIG_PATH KEYS_CONFIG 
#define USER_VIDEO_CONFIG_PATH USER_CONFIG_PATH VIDEO_CONFIG
#define USER_INPUT_CONFIG_PATH USER_CONFIG_PATH INPUT_CONFIG

const char* get_video_config_path(en_config_type config_type) {
    switch (config_type) {
    case en_config_base:
    default:
        return BASE_VIDEO_CONFIG_PATH;
    case en_config_user:
        return USER_VIDEO_CONFIG_PATH;
    }
}

const char* get_input_config_path(en_config_type config_type) {
    switch (config_type) {
    case en_config_base:
    default:
        return BASE_INPUT_CONFIG_PATH;
    case en_config_user:
        return USER_INPUT_CONFIG_PATH;
    }
}

const char* get_keys_config_path(en_config_type config_type) {
    switch (config_type) {
    case en_config_base:
    default:
        return BASE_KEYS_CONFIG_PATH;
    case en_config_user:
        return USER_KEYS_CONFIG_PATH;
    }
}

void
en_settings::merge_with(en_settings other) {
    input.merge_with(other.input);
    video.merge_with(other.video);
    bindings.merge_with(other.bindings);
}

en_settings en_settings::get_delta(en_settings other) {
    en_settings delta;
    delta.input    = input.get_delta(other.input);
    delta.video    = video.get_delta(other.video);
    delta.bindings = bindings.get_delta(other.bindings);

    return delta;
}


// todo: consolidate save_n_settings into overloads?
void
save_settings(en_settings to_save) {
    en_settings base = load_settings(en_config_base);

    en_settings delta = base.get_delta(std::move(to_save));

    save_binding_settings(delta.bindings);
    save_video_settings(delta.video);
    save_input_settings(delta.input);
}

void
save_binding_settings(binding_settings to_save) {
    config_t bindings_config{};
    config_setting_t *binds;
    config_setting_t *root;

    config_init(&bindings_config);

    root = config_root_setting(&bindings_config);

    binds = config_setting_add(root, "binds", CONFIG_TYPE_LIST);

    for (auto &actionPair : to_save.bindings) {
        auto action = actionPair.first;
        auto action_string = lookup_input_action(action);
        auto action_input = actionPair.second;

        if (action == action_invalid) {
            continue;
        }

        auto action_entry = config_setting_add(binds, nullptr, CONFIG_TYPE_GROUP);

        auto action_config = config_setting_add(action_entry, "action", CONFIG_TYPE_STRING);
        config_setting_set_string(action_config, action_string);

        auto inputs_config = config_setting_add(action_entry, "inputs", CONFIG_TYPE_ARRAY);
        for (auto &bind : action_input.binds.inputs) {
            auto input = lookup_input(bind);
            if (bind != input_invalid) {
                auto bind_entry = config_setting_add(inputs_config, nullptr, CONFIG_TYPE_STRING);
                config_setting_set_string(bind_entry, input);
            }
        }
    }

    // of course it worked, what could go wrong?
    config_write_file(&bindings_config, USER_KEYS_CONFIG_PATH);

    config_destroy(&bindings_config);
}

void
save_video_settings(video_settings to_save) {
    config_t video_config{};
    config_setting_t *video;
    config_setting_t *root;

    config_init(&video_config);

    root = config_root_setting(&video_config);

    video = config_setting_add(root, "video", CONFIG_TYPE_GROUP);

    if (to_save.mode != window_mode::invalid) {
        auto mode = config_setting_add(video, "mode", CONFIG_TYPE_STRING);
        config_setting_set_window_mode(mode, to_save.mode);
    }

    // of course it worked, what could go wrong?
    config_write_file(&video_config, USER_VIDEO_CONFIG_PATH);

    config_destroy(&video_config);
}

void
save_input_settings(input_settings to_save) {
    config_t input_config{};
    config_setting_t *input;
    config_setting_t *root;

    config_init(&input_config);

    root = config_root_setting(&input_config);

    input = config_setting_add(root, "input", CONFIG_TYPE_GROUP);

    if (to_save.mouse_invert != INVALID_SETTINGS_FLOAT) {
        auto invert_config = config_setting_add(input, "mouse_invert", CONFIG_TYPE_FLOAT);
        config_setting_set_float(invert_config, to_save.mouse_invert);
    }

    if (to_save.mouse_x_sensitivity != INVALID_SETTINGS_FLOAT) {
        auto x_sens_config = config_setting_add(input, "mouse_x_sensitivity", CONFIG_TYPE_FLOAT);
        config_setting_set_float(x_sens_config, to_save.mouse_x_sensitivity);
    }

    if (to_save.mouse_y_sensitivity != INVALID_SETTINGS_FLOAT) {
        auto y_sens_config = config_setting_add(input, "mouse_y_sensitivity", CONFIG_TYPE_FLOAT);
        config_setting_set_float(y_sens_config, to_save.mouse_y_sensitivity);
    }

    // of course it worked, what could go wrong?
    config_write_file(&input_config, USER_INPUT_CONFIG_PATH);

    config_destroy(&input_config);
}

// todo: consolidate load_n_settings into overloads and pass and fill references?
en_settings
load_settings(en_config_type config_type) {
    en_settings loaded_settings;

    loaded_settings.input    = load_input_settings(config_type);
    loaded_settings.bindings = load_binding_settings(config_type);
    loaded_settings.video    = load_video_settings(config_type);

    return loaded_settings;
}

binding_settings
load_binding_settings(en_config_type config_type) {
    binding_settings loaded_bindings;
    config_t cfg{};
    config_setting_t *binds_config_setting = nullptr;

    const char* config_path = get_keys_config_path(config_type);

    config_init(&cfg);

    if (!config_read_file(&cfg, config_path))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg), config_path);
        config_destroy(&cfg);

        return loaded_bindings;
    }

    binds_config_setting = config_lookup(&cfg, "binds");

    if (binds_config_setting != nullptr) {
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

            const char **inputs_names = nullptr;
            const char *action_name = nullptr;
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
            loaded_bindings.bindings[i_action] = action(i_action);

            for (input_index = 0; input_index < inputs_count; ++input_index) {
                en_input input = lookup_input(inputs_names[input_index]);
                loaded_bindings.bindings[i_action].bind(input);
            }

            if (inputs)
                free(inputs);
        }
    }

    return loaded_bindings;
}

video_settings
load_video_settings(en_config_type config_type) {
    video_settings loaded_video;
    config_t cfg{};
    config_setting_t *video_config_setting = nullptr;

    const char* config_path = get_video_config_path(config_type);

    config_init(&cfg);

    if (!config_read_file(&cfg, config_path))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
               config_error_line(&cfg), config_error_text(&cfg), config_path);
        config_destroy(&cfg);

        return loaded_video;
    }

    video_config_setting = config_lookup(&cfg, "video");

    if (video_config_setting != nullptr) {
        window_mode mode = window_mode::windowed;

        /* window_mode */
        int success = config_setting_lookup_window_mode(
            video_config_setting, "mode", &mode);

        if (success == CONFIG_TRUE) {
            loaded_video.mode = mode;
        }
    }

    return loaded_video;
}

input_settings
load_input_settings(en_config_type config_type) {
    input_settings loaded_inputs;
    config_t cfg{};
    config_setting_t *input_config_setting = nullptr;

    const char* config_path = get_input_config_path(config_type);

    config_init(&cfg);

    if (!config_read_file(&cfg, config_path))
    {
        printf("%s:%d - %s reading %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg), config_path);
        config_destroy(&cfg);

        return loaded_inputs;
    }

    input_config_setting = config_lookup(&cfg, "input");

    if (input_config_setting != nullptr) {
        double mouse_invert        = 0.0;
        double mouse_x_sensitivity = 0.0;
        double mouse_y_sensitivity = 0.0;

        /* mouse_invert */
        int success = config_setting_lookup_float(
            input_config_setting, "mouse_invert", &mouse_invert);

        if (success == CONFIG_TRUE) {
            loaded_inputs.mouse_invert = (float)mouse_invert;
        }

        /* mouse_x_sensitivity */
        success = config_setting_lookup_float(
            input_config_setting, "mouse_x_sensitivity", &mouse_x_sensitivity);

        if (success == CONFIG_TRUE) {
            loaded_inputs.mouse_x_sensitivity = (float)mouse_x_sensitivity;
        }

        /* mouse_y_sensitivity */
        success = config_setting_lookup_float(
            input_config_setting, "mouse_y_sensitivity", &mouse_y_sensitivity);

        if (success == CONFIG_TRUE) {
            loaded_inputs.mouse_y_sensitivity = (float)mouse_x_sensitivity;
        }
    }
    
    return loaded_inputs;
}
