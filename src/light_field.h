#pragma once

#include <glm/glm.hpp>
#include <epoxy/gl.h>
#include <algorithm>

#include "block.h"
#include "ship_space.h"

struct light_field {
    GLuint texobj;
    unsigned char data[128 * 128 * 128];

    light_field() : texobj(0) {
        glGenTextures(1, &texobj);
        glBindTexture(GL_TEXTURE_3D, texobj);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_R8, 128, 128, 128);
    }

    void bind(int texunit)
    {
        glActiveTexture(GL_TEXTURE0 + texunit);
        glBindTexture(GL_TEXTURE_3D, texobj);
    }

    void upload()
    {
        /* TODO: toroidal addressing + partial updates, to make these uploads VERY small */
        /* TODO: experiment with buffer texture rather than 3D, so we can have the light field
        * persistently mapped in our address space */

        /* DSA would be nice -- for now, we'll just disturb the tex0 binding */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, texobj);

        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
            128, 128, 128,
            GL_RED,
            GL_UNSIGNED_BYTE,
            data);
    }
};


const int light_atten = 50;
/* as far as we can ever light from a light source */
const int max_light_prop = (255 + light_atten - 1) / light_atten;

static bool need_lightfield_update = false;
static glm::ivec3 lightfield_update_mins;
static glm::ivec3 lightfield_update_maxs;

extern light_field *light;


void set_light_level(int x, int y, int z, int level);


unsigned char get_light_level(int x, int y, int z);


void mark_lightfield_update(glm::ivec3 center);


void update_lightfield(ship_space *ship);
