#include "text.h"

#include <algorithm>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdlib.h>

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif


#define TEXT_ATLAS_WIDTH    512
#define TEXT_ATLAS_HEIGHT   512


text_renderer::text_renderer(char const *font, int size)
    : bo(0), bo_vertex_count(0), bo_capacity(0), vao(0), verts()
{
    /* load the font into metrics array + texture */
    atlas = new texture_atlas(1, TEXT_ATLAS_WIDTH, TEXT_ATLAS_HEIGHT);   /* text will be 1 channel, 8 bit */

    FT_Library ft_library;
    if (FT_Init_FreeType(&ft_library))
        errx(1, "freetype init failed");

    FT_Face ft_face;
    if (FT_New_Face(ft_library, font, 0, &ft_face))
        errx(1, "failed to load font `%s`", font);

    FT_Set_Pixel_Sizes(ft_face, 0, size);

    for (int i = 0; i < 256; i++) {
        metrics *m = &ms[i];

        /* load one glyph */
        FT_Load_Char(ft_face, i, FT_LOAD_TARGET_NORMAL | FT_LOAD_RENDER);
        FT_Bitmap const *bitmap = &ft_face->glyph->bitmap;

        atlas->add_bitmap(bitmap->buffer, bitmap->pitch, bitmap->width, bitmap->rows,
            &m->x, &m->y);

        m->w = bitmap->width;
        m->h = bitmap->rows;

        m->xoffset = (float) ft_face->glyph->bitmap_left;
        m->advance = ft_face->glyph->linearHoriAdvance / 65536.0f;   /* 16.16 -> float */
        m->yoffset = (float) ft_face->glyph->bitmap_top;
    }

    atlas->upload();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &bo);
    glBindBuffer(GL_ARRAY_BUFFER, bo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(text_vertex), (GLvoid const *)offsetof(text_vertex, x));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(text_vertex), (GLvoid const *)offsetof(text_vertex, u));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(text_vertex), (GLvoid const *)offsetof(text_vertex, r));

    printf("Loaded font %s at size %d\n", font, size);
}


void
text_renderer::add(char const *str, float x, float y, float r, float g, float b)
{
    float xx = x;

    for (; *str; str++) {
        if (*str == '\n') {
            y -= 24;
            xx = x;
            continue;
        }

        metrics *m = &ms[(unsigned) *str];

        float u0 = m->x / (float)TEXT_ATLAS_WIDTH;
        float u1 = (m->x + m->w) / (float)TEXT_ATLAS_WIDTH;
        float v0 = m->y / (float)TEXT_ATLAS_HEIGHT;
        float v1 = (m->y + m->h) / (float)TEXT_ATLAS_HEIGHT;

        xx += m->xoffset;
        float yy = y + m->yoffset;

        text_vertex p0 = { xx, yy, u0, v0, r, g, b };
        text_vertex p1 = { xx + m->w, yy, u1, v0, r, g, b };
        text_vertex p2 = { xx + m->w, yy - m->h, u1, v1, r, g, b };
        text_vertex p3 = { xx, yy - m->h, u0, v1, r, g, b };

        verts.push_back(p0);
        verts.push_back(p1);
        verts.push_back(p2);

        verts.push_back(p0);
        verts.push_back(p2);
        verts.push_back(p3);

        xx += m->advance;
    }
}


void
text_renderer::measure(char const *str, float *x, float *y)
{
    float xx = *x;

    for (; *str; str++) {
        if (*str == '\n') {
            *x = std::max(*x, xx);
            xx = 0;
            continue;
        }

        metrics *m = &ms[(unsigned) *str];

        xx += m->xoffset + (str[1] && str[1] != '\n') ? m->advance : (m->xoffset + m->w);
    }

    *x = std::max(*x, xx);
}


void
text_renderer::upload()
{
    glBindBuffer(GL_ARRAY_BUFFER, bo);

    /* Exact-fit is pretty lousy as a growth strategy, but oh well */
    if (verts.size() > bo_capacity) {
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(text_vertex), &verts[0], GL_STREAM_DRAW);
        bo_capacity = verts.size();
        bo_vertex_count = verts.size();
    }
    else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(text_vertex), &verts[0]);
        bo_vertex_count = verts.size();
    }
}


void
text_renderer::reset()
{
    verts.clear();
}


void
text_renderer::draw()
{
    glBindVertexArray(vao);
    atlas->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, bo_vertex_count);
}
