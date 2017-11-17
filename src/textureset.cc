#include <stdio.h>

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif

#include <epoxy/gl.h>
#include <SDL.h>
#include <SDL_image.h>

#include "common.h"
#include "textureset.h"


texture_set::texture_set(GLenum target, int dim, int array_size)
    : texobj(0), dim(dim), array_size(array_size), target(target) {
    glGenTextures(1, &texobj);
    glBindTexture(target, texobj);
    if (target == GL_TEXTURE_CUBE_MAP) {
        glTexStorage2D(target, 1, GL_RGBA8, dim, dim);
    }
    else {
        glTexStorage3D(target,
            1,   /* no mips! I WANT YOUR EYES TO BLEED -- todo, fix this. */
            GL_RGBA8, dim, dim, array_size);
    }
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


void
texture_set::bind(int texunit)
{
    glActiveTexture(GL_TEXTURE0 + texunit);
    glBindTexture(target, texobj);
}


void
texture_set::load(int slot, char const *filename)
{
    SDL_Surface* surf = IMG_Load( filename );

    if (!surf)
        errx(1, "Failed to load texture %d:%s", slot, filename);
    if (surf->w != dim || surf->h != dim)
        errx(1, "Texture %d:%s is the wrong size: %dx%d but expected %dx%d",
                slot, filename, surf->w, surf->h, dim, dim);

    /* bring on DSA... for now, we disturb the tex0 binding */
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(target, texobj);
    if (target == GL_TEXTURE_CUBE_MAP) {
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + slot, 0,
                        0, 0, dim, dim,
                        surf->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB,
                        GL_UNSIGNED_BYTE,
                        surf->pixels);
    }
    else {
        glTexSubImage3D(target, 0,
                        0, 0, slot,
                        dim, dim, 1,
                        surf->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB,
                        GL_UNSIGNED_BYTE,
                        surf->pixels);
    }

    SDL_FreeSurface(surf);
}
