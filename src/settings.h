#pragma once

struct video_settings {
    /* render xres */           /* render framebuffer to this size (usually window size) */
    /* render yres */           /* render framebuffer to this size (usually window size) */
    /* framebuffer xres */      /* for super/sub sampling? */
    /* framebuffer yres*/       /* for super/sub sampling? */
    /* fancy_render_shader */   /* retro, crt, scanline, other effects? */
    /* shadows */               /* shadows on or off/quality */
    /* anisotropy */            /* less eye bleed */
    /* antialiasing */          /* SMAA, CSAAS, FXAA, ETCAA*/
};

struct input_settings {
    bool mouse_invert = false;
    float mouse_x_sensitivity = -0.001f;
    float mouse_y_sensitivity = -0.001f;
};

struct settings {
    video_settings video;
    input_settings input;
};
