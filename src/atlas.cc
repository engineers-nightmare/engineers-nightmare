#include "text.h"

#include <algorithm>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif


static GLenum
channels_to_internalformat(unsigned channels)
{
    switch (channels) {
        case 1: return GL_R8;
        case 4: return GL_RGBA8;
        default: errx(1, "unsupported channel count");
    }
}


static GLenum
channels_to_clientformat(unsigned channels)
{
    switch (channels) {
        case 1: return GL_RED;
        case 4: return GL_RGBA;
        default: errx(1, "unsupported channel count");
    }
}


texture_atlas::texture_atlas(unsigned channels, unsigned width, unsigned height)
    : tex(0), x(0), y(0), h(0), channels(channels), width(width), height(height)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, channels_to_internalformat(channels),
                   width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    buf = new unsigned char[width * height * channels];
}


void
texture_atlas::add_bitmap(unsigned char *src, int pitch, unsigned width, unsigned height, int *out_x, int *out_y)
{
    /* overflow to next row */
    if (x + width > this->width) {
        y += h;
        x = 0;
        h = 0;
    }

    /* adjust height of atlas row */
    h = std::max(h, height);

    unsigned char *dest = buf + (x + y * this->width) * channels;

    for (unsigned int r = 0; r < height; r++) {
        memcpy(dest, src, width * channels);
        src += pitch;
        dest += this->width * channels;
    }

    *out_x = x;
    *out_y = y;

    x += width;
}


void
texture_atlas::upload()
{
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                    channels_to_clientformat(channels), GL_UNSIGNED_BYTE, buf);
}


void
texture_atlas::bind(int texunit)
{
    glActiveTexture(GL_TEXTURE0 + texunit);
    glBindTexture(GL_TEXTURE_2D, tex);
}



