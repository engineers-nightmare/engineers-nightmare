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

struct text_renderer
{
    text_renderer(char const *font, int size);

    metrics ms[256];

    GLuint tex;
    GLuint bo;
    GLuint bo_vertex_count;
    GLuint bo_capacity;
    GLuint vao;

    std::vector<text_vertex> verts;

    void add(char const *str, float x, float y);
    void measure(char const *str, float *x, float *y);
    void upload();
    void reset();
    void draw();
};
