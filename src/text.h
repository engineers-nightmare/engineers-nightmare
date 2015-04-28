#pragma once

#include <epoxy/gl.h>
#include <vector>

struct metrics
{
    int x, y, w, h;

    /* glyph metrics relative to baseline */
    float xoffset, advance, yoffset;
};


struct text_vertex
{
    float x, y;
    float u, v;
    float r, g, b;
};


struct texture_atlas
{
    GLuint tex;
    unsigned x, y, h;
    unsigned char *buf;

    texture_atlas();
    void add_bitmap(unsigned char *src, int pitch, unsigned width, unsigned height, int *out_x, int *out_y);
    void upload();
    void bind();
};


struct text_renderer
{
    text_renderer(char const *font, int size);

    metrics ms[256];

    GLuint bo;
    GLuint bo_vertex_count;
    GLuint bo_capacity;
    GLuint vao;

    std::vector<text_vertex> verts;

    texture_atlas *atlas;

    void add(char const *str, float x, float y);
    void measure(char const *str, float *x, float *y);
    void upload();
    void reset();
    void draw();
};
