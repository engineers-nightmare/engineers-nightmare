#pragma once

#include <unordered_map>
#include <limits.h>
#include <float.h>

#include "input.h"

/* to support merging, we need a way of defaulting values
 * to indicate not present, rather than just set to same value
 * bool might be tricky...
 */
#define INVALID_SETTINGS_INT INT_MAX
#define INVALID_SETTINGS_FLOAT FLT_MAX

struct video_settings {
    /* render xres */           /* render framebuffer to this size (usually window size) */
    /* render yres */           /* render framebuffer to this size (usually window size) */
    /* framebuffer xres */      /* for super/sub sampling? */
    /* framebuffer yres*/       /* for super/sub sampling? */
    /* fancy_render_shader */   /* retro, crt, scanline, other effects? */
    /* shadows */               /* shadows on or off/quality */
    /* anisotropy */            /* less eye bleed */
    /* antialiasing */          /* SMAA, CSAAS, FXAA, ETCAA*/

    void merge_with(video_settings);
};

struct input_settings {
    float mouse_invert        = INVALID_SETTINGS_FLOAT;
    float mouse_x_sensitivity = INVALID_SETTINGS_FLOAT;
    float mouse_y_sensitivity = INVALID_SETTINGS_FLOAT;

    void merge_with(input_settings);
};

struct binding_settings {
    std::unordered_map<en_action, action, std::hash<int>> bindings;

    void merge_with(binding_settings);
};

struct settings {
    video_settings video;
    input_settings input;
    binding_settings bindings;

    void merge_with(settings);
};
