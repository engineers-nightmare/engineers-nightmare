#include "settings.h"

#include <algorithm>

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
    if (other.mode != window_mode::invalid)
        this->mode = other.mode;

    if (other.fov != INVALID_SETTINGS_FLOAT)
        this->fov = other.fov;
}

input_settings input_settings::get_delta(input_settings other) {
    // relies on fields being initialized to INVALID_SETTINGS_{type}
    input_settings delta;

    if (other.mouse_invert != mouse_invert) {
        delta.mouse_invert = other.mouse_invert;
    }

    if (other.mouse_x_sensitivity != mouse_x_sensitivity) {
        delta.mouse_x_sensitivity = other.mouse_x_sensitivity;
    }

    if (other.mouse_y_sensitivity != mouse_y_sensitivity) {
        delta.mouse_y_sensitivity = other.mouse_y_sensitivity;
    }

    return delta;
}

binding_settings binding_settings::get_delta(binding_settings other) {
    // relies on fields being initialized to INVALID_SETTINGS_{type}
    binding_settings delta;

    // 1. if bind for action exists in base, and exists in other, set delta to other
    //    when any of other is different from base
    //    should cover for unbound as well. That is, removing a bind from base, but
    //    not reassigning (special case of input_unbound)
    // 2. if bind for action exists in base, and does not exist in other, don't set in delta
    //    This might actually be really weird. other ?should? never not contain something 
    //    that exists in base
    // 3. if bind for action does not exist in base, but does exist in other, set delta to
    //    other

    // first go through all of this bindings for above cases 1, 2
    for (auto thisBind : bindings) {
        auto inputAction = thisBind.first;

        // case 2
        auto findOther = other.bindings.find(inputAction);
        if (findOther == other.bindings.end()) {
            continue;
        }

        auto thisAction  = thisBind.second;
        auto thisInputs  = thisAction.binds.inputs;
        auto otherAction = findOther->second;
        auto otherInputs = otherAction.binds.inputs;

        // case 1
        for (auto otherInput : otherInputs) {
            auto thisInput = find(thisInputs.begin(), thisInputs.end(), otherInput);
            if (thisInput == thisInputs.end()) {
                delta.bindings.insert(*findOther);
                break;
            }
        }
    }

    for (auto otherBind : other.bindings) {
        auto input = otherBind.first;

        auto findThis = bindings.find(input);
        if (findThis != bindings.end()) {
            // exists in this, already handled
            continue;
        }

        // case 3
        delta.bindings.insert(otherBind);
    }

    return delta;
}

video_settings video_settings::get_delta(video_settings other) {
    video_settings delta;

    if (other.mode != mode) {
        delta.mode = other.mode;
    }

    if (other.fov != fov) {
        delta.fov = other.fov;
    }

    return delta;
}
