#pragma once

#include <unordered_map>
#include <limits.h>
#include <float.h>

#include "input.h"
#include "enums/enums.h"

/* to support merging, we need a way of defaulting values
 * to indicate not present, rather than just set to same value
 * bool might be tricky...
 */
#define INVALID_SETTINGS_INT INT_MAX
#define INVALID_SETTINGS_FLOAT FLT_MAX
#define INVALID_SETTINGS_ENUM INT_MAX

template <typename T>
struct settings {
    virtual ~settings() = default;

    virtual void merge_with(const T &) = 0;
    virtual T get_delta(const T &) const = 0;
};

struct audio_settings : settings<audio_settings> {
    /* music/sound/etc volumes */
    /* num channels */

    float global_volume = INVALID_SETTINGS_FLOAT;

    void merge_with(const audio_settings&) override;
    audio_settings get_delta(const audio_settings &) const override;
};

struct video_settings : settings<video_settings> {
    /* render xres */           /* render framebuffer to this size (usually window size) */
    /* render yres */           /* render framebuffer to this size (usually window size) */
    /* framebuffer xres */      /* for super/sub sampling? */
    /* framebuffer yres*/       /* for super/sub sampling? */
    /* fancy_render_shader */   /* retro, crt, scanline, other effects? */
    /* shadows */               /* shadows on or off/quality */
    /* anisotropy */            /* less eye bleed */
    /* antialiasing */          /* SMAA, CSAAS, FXAA, ETCAA*/

    window_mode mode = window_mode::invalid;

    float fov = INVALID_SETTINGS_FLOAT;

    void merge_with(const video_settings &) override;
    video_settings get_delta(const video_settings &) const override;
};

struct input_settings : settings<input_settings> {
    float mouse_invert        = INVALID_SETTINGS_FLOAT;
    float mouse_x_sensitivity = INVALID_SETTINGS_FLOAT;
    float mouse_y_sensitivity = INVALID_SETTINGS_FLOAT;

    void merge_with(const input_settings &) override;
    input_settings get_delta(const input_settings &) const override;
};

struct binding_settings : settings<binding_settings> {
    std::unordered_map<en_action, action, std::hash<int>> bindings;

    void merge_with(const binding_settings &) override;
    binding_settings get_delta(const binding_settings &) const override;
};

struct en_settings : settings<en_settings> {
    audio_settings audio;
    video_settings video;
    input_settings input;
    binding_settings bindings;

    void merge_with(const en_settings &) override;
    en_settings get_delta(const en_settings &) const override;
};
