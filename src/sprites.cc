#include <epoxy/gl.h>
#include <SDL.h>
#include <SDL_image.h>

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif

#include "text.h"


sprite_renderer::sprite_renderer()
    : bo(0), bo_vertex_count(0), bo_capacity(0), vao(0), verts()
{
    atlas = new texture_atlas(4, 512, 512);   /* text will be 4 channels, 8 bit */

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &bo);
    glBindBuffer(GL_ARRAY_BUFFER, bo);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(sprite_vertex), (GLvoid const *)offsetof(sprite_vertex, x));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(sprite_vertex), (GLvoid const *)offsetof(sprite_vertex, u));
}


sprite_metrics
sprite_renderer::load(char const *filename)
{
    SDL_Surface* surf = IMG_Load( filename );
    if (!surf)
        errx(1, "Failed to load %s\n", filename);
    if (surf->format->BytesPerPixel != 4)
        errx(1, "Bad mage format for %s\n", filename);

    sprite_metrics m;
    m.w = surf->w;
    m.h = surf->h;
    atlas->add_bitmap((unsigned char *)surf->pixels, surf->w * surf->format->BytesPerPixel, surf->w, surf->h, &m.x, &m.y);

    SDL_FreeSurface(surf);

    /* we can do this less if we know when we're finished building the atlas */
    atlas->upload();
    return m;
}


void
sprite_renderer::add(sprite_metrics const *m, float x, float y)
{
    float u0 = m->x / (float)atlas->width;
    float u1 = (m->x + m->w) / (float)atlas->width;
    float v0 = m->y / (float)atlas->height;
    float v1 = (m->y + m->h) / (float)atlas->height;

    sprite_vertex p0 = { x, y, u0, v0 };
    sprite_vertex p1 = { x + m->w, y, u1, v0 };
    sprite_vertex p2 = { x + m->w, y - m->h, u1, v1 };
    sprite_vertex p3 = { x, y - m->h, u0, v1 };

    verts.push_back(p0);
    verts.push_back(p1);
    verts.push_back(p2);

    verts.push_back(p0);
    verts.push_back(p2);
    verts.push_back(p3);
}


void
sprite_renderer::upload()
{
    glBindBuffer(GL_ARRAY_BUFFER, bo);

    /* Exact-fit is pretty lousy as a growth strategy, but oh well */
    if (verts.size() > bo_capacity) {
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(sprite_vertex), &verts[0], GL_STREAM_DRAW);
        bo_capacity = (GLuint)verts.size();
        bo_vertex_count = (GLuint)verts.size();
    }
    else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(sprite_vertex), &verts[0]);
        bo_vertex_count = (GLuint)verts.size();
    }
}


void
sprite_renderer::reset()
{
    verts.clear();
}


void
sprite_renderer::draw()
{
    glBindVertexArray(vao);
    atlas->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, bo_vertex_count);
}
