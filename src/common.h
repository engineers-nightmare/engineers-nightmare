#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef _WIN32
#define __unused __attribute__(( unused ))
#else
#define __unused
#endif // _WIN32

#define PI 3.14159265f
#define DEG2RAD(x) (x * PI / 180.f)
#define RAD2DEG(x) (x * 180.f / PI)

#define RENDER_DIM 1024
#define WORLD_TEXTURE_DIMENSION     32
#define MAX_WORLD_TEXTURES          64


static inline glm::ivec3
get_coord_containing(glm::vec3 v) {
    /* truncating via int cast */
    int x = (int)v.x; if (v.x < 0) x--;
    int y = (int)v.y; if (v.y < 0) y--;
    int z = (int)v.z; if (v.z < 0) z--;

    return glm::ivec3(x, y, z);
}

template<typename T>
T
clamp(T t, T lower, T upper) {
    if (t < lower) return lower;
    if (t > upper) return upper;
    return t;
}

struct raycast_info {
    bool hit;
    bool inside;
    glm::ivec3 bl;          /* the block we hit */
    glm::ivec3 n;           /* the face normal we hit */
    glm::ivec3 p;           /* the block along the normal */
    struct block *block;
};

static inline unsigned
normal_to_surface_index(raycast_info const *rc)
{
    if (rc->n.x == 1) return 0;
    if (rc->n.x == -1) return 1;
    if (rc->n.y == 1) return 2;
    if (rc->n.y == -1) return 3;
    if (rc->n.z == 1) return 4;
    if (rc->n.z == -1) return 5;

    return 0;   /* unreachable */
}

static inline glm::ivec3
surface_index_to_normal(int index)
{
    glm::ivec3 n(0, 0, 0);

    switch (index) {
        case 0: n.x = 1; break;
        case 1: n.x = -1; break;
        case 2: n.y = 1; break;
        case 3: n.y = -1; break;
        case 4: n.z = 1; break;
        case 5: n.z = -1; break;
    }

    return n;
}

static inline glm::mat4
mat_rotate_mesh(glm::vec3 pt, glm::vec3 dir) {
    auto abs_normal = glm::abs(dir);
    glm::vec3 temp_up(0, 0, 1);
    if (abs_normal.z > abs_normal.y && abs_normal.z > abs_normal.x) {
        /* avoid degeneracy at the `poles` */
        temp_up = glm::vec3(0, 1, 0);
    }
    glm::mat4 m = glm::transpose(glm::lookAt(dir, glm::vec3(0, 0, 0), temp_up));
    m[3][0] = pt.x;
    m[3][1] = pt.y;
    m[3][2] = pt.z;
    m[0][3] = 0;
    m[1][3] = 0;
    m[2][3] = 0;
    return m;
}

static inline glm::mat4
mat_block_face(glm::ivec3 p, int face)
{
    auto norm = glm::vec3(surface_index_to_normal(face));
    auto pos = glm::vec3(p) + glm::vec3(0.5f) + 0.5f * norm;
    return mat_rotate_mesh(pos, -norm);
}

static inline glm::mat4
mat_position(glm::vec3 pos)
{
    return glm::translate(glm::mat4(1), pos);
}

static inline glm::mat4
mat_scale(glm::vec3 scale) {
    return glm::scale(glm::mat4(1), scale);
}

static inline glm::mat4
mat_scale(float x, float y, float z) {
    return mat_scale(glm::vec3(x, y, z));
}
