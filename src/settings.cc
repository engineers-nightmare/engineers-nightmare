#include "settings.h"

void input_settings::merge_with(input_settings other) {
    // bool mouse_invert
    // float mouse_x_sensitivity
    // float mouse_y_sensitivity

    if (other.mouse_invert != INVALID_SETTINGS_FLOAT)
        this->mouse_invert = other.mouse_invert;

    if (other.mouse_x_sensitivity != INVALID_SETTINGS_FLOAT)
        this->mouse_x_sensitivity = other.mouse_x_sensitivity;

    if (other.mouse_y_sensitivity != INVALID_SETTINGS_FLOAT)
        this->mouse_y_sensitivity = other.mouse_y_sensitivity;
}

void binding_settings::merge_with(binding_settings other) {
    for (auto &actionPair : other.bindings) {
        auto input = actionPair.first;
        auto action = &actionPair.second;
        auto binds = &action->binds;

        bool cleared_this = false;

        for (auto &bind : binds->inputs) {
            binding* thisBinds = &this->bindings[input].binds;
            auto contained =
                std::find(thisBinds->inputs.begin(), thisBinds->inputs.end(), bind)
                != thisBinds->inputs.end();

            /* clear all bindings on this as we have some from other */
            if (cleared_this != true) {
                thisBinds->inputs.clear();
                cleared_this = true;
            }

            /* if unbound, clear all bindings on this and don't process further binds
             * for this input
             */
            if (bind == input_unbound) {
                thisBinds->inputs.clear();
                this->bindings.erase(input);
                break;
            }

            /* add binds from other to this */
            thisBinds->inputs.push_back(bind);
        }
    }
}

void video_settings::merge_with(video_settings other) {
    /* nothing yet */
}
