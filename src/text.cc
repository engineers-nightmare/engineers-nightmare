#include "text.h"

#include <algorithm>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdlib.h>
#include <err.h>


#define TEXT_ATLAS_WIDTH    512
#define TEXT_ATLAS_HEIGHT   512


text_renderer::text_renderer(char const *font, int size)
    : tex(0), bo(0), bo_vertex_count(0), bo_capacity(0), vao(0), verts()
{
    /* load the font into metrics array + texture */
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, TEXT_ATLAS_WIDTH, TEXT_ATLAS_HEIGHT);

    FT_Library ft_library;
    if (FT_Init_FreeType(&ft_library))
        errx(1, "freetype init failed");

    FT_Face ft_face;
    if (FT_New_Face(ft_library, font, 0, &ft_face))
        errx(1, "failed to load font `%s`", font);

    FT_Set_Pixel_Sizes(ft_face, 0, size);

    unsigned char *buf = new unsigned char[TEXT_ATLAS_WIDTH * TEXT_ATLAS_HEIGHT];

    unsigned x = 0;
    unsigned y = 0;
    unsigned h = 0;

    for (int i = 0; i < 256; i++) {
        metrics *m = &ms[i];

        /* load one glyph */
        FT_Load_Char(ft_face, i, FT_LOAD_TARGET_NORMAL | FT_LOAD_RENDER);
        FT_Bitmap const *bitmap = &ft_face->glyph->bitmap;

        /* overflow to next row */
        if (x + bitmap->width > TEXT_ATLAS_WIDTH) {
            y += h;
            x = 0;
            h = 0;
        }

        /* adjust height of atlas row */
        h = std::max(h, bitmap->rows);

        unsigned char *src = bitmap->buffer;
        unsigned char *dest = buf + x + y * TEXT_ATLAS_WIDTH;

        for (int r = 0; r < bitmap->rows; r++) {
            memcpy(dest, src, bitmap->width);
            src += bitmap->pitch;
            dest += TEXT_ATLAS_WIDTH;
        }

        m->x = x;
        m->y = y;
        m->w = bitmap->width;
        m->h = bitmap->rows;

        m->xoffset = ft_face->glyph->bitmap_left;
        m->advance = ft_face->glyph->linearHoriAdvance / 65536.0f;   /* 16.16 -> float */
        m->yoffset = ft_face->glyph->bitmap_top;

        x += bitmap->width;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXT_ATLAS_WIDTH, TEXT_ATLAS_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, buf);
    delete [] buf;

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
text_renderer::add(char const *str, float x, float y)
{
    float r = 1, g = 1, b = 1;  /* initial color */

    float xx = x;

    for (; *str; str++) {
        if (*str == '\n') {
            y -= 24;
            xx = x;
            continue;
        }

        metrics *m = &ms[*str];

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

        metrics *m = &ms[*str];

        xx += m->xoffset + m->advance;
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
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays(GL_TRIANGLES, 0, bo_vertex_count);
}
