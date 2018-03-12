#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "timer.h"
#include "component/c_entity.h"

#ifndef _WIN32
#define __unused __attribute__(( unused ))
#else
#define __unused
#endif // _WIN32

#define RENDER_DIM 1024
#define WORLD_TEXTURE_DIMENSION     32

#define MAX_NETWORKS 10

#define MAX_REACH_DISTANCE 5.0f


struct frame_info {
    Timer timer{};

    const float fps_duration = 0.25f;

    unsigned frame = 0;

    unsigned fps_frame = 0;
    double fps_time = 0.0;

    double dt = 0.0;
    double fps = 0.0;
    double elapsed = 0.0;

    void tick() {
        auto t = timer.touch();

        dt = (float) t.delta;   /* narrowing */
        frame++;
        elapsed += dt;

        fps_frame++;
        fps_time += dt;

        if (fps_time >= fps_duration) {
            fps = 1 / (fps_time / fps_frame);
            fps_time = 0.f;
            fps_frame = 0;
        }
    }
};

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

struct raycast_info_block {
    bool hit;
    bool inside;
    glm::ivec3 bl;          /* the block we hit */
    glm::ivec3 n;           /* the face normal we hit */
    glm::ivec3 p;           /* the block along the normal */
    struct block *block;
    float t;                /* distance along the ray */
    glm::vec3 hitCoord;     /* intersection point in ship space */
};

struct raycast_info_world {
    bool hit;
    c_entity entity;
    glm::vec3 hitCoord;
    glm::vec3 hitNormal;
};

struct raycast_info {
    raycast_info_block block;
    raycast_info_world world;
};

static inline unsigned
normal_to_surface_index(int nx, int ny, int nz)
{
    if (nx == 1) return 0;
    if (nx == -1) return 1;
    if (ny == 1) return 2;
    if (ny == -1) return 3;
    if (nz == 1) return 4;
    if (nz == -1) return 5;

    return 0;   /* unreachable */
}

static inline unsigned
normal_to_surface_index(raycast_info_block const *rc)
{
    return normal_to_surface_index(rc->n.x, rc->n.y, rc->n.z);
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
mat_block_face(glm::vec3 p, int face)
{
    auto norm = glm::vec3(surface_index_to_normal(face));
    auto pos = p + glm::vec3(0.5f) + 0.5f * norm;
    return mat_rotate_mesh(pos, -norm);
}

static inline glm::mat4
mat_block_surface(glm::vec3 p, int face)
{
    auto norm = glm::vec3(surface_index_to_normal(face));
    auto pos = p + glm::vec3(0.5f) + 0.5f * -norm;
    return mat_rotate_mesh(pos, norm);
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

static inline constexpr uint32_t fourcc(char const tag[5]) {
    return (unsigned char)(tag[0] << 24) |
        (unsigned char)(tag[1] << 16) |
        (unsigned char)(tag[2] << 8) |
        (unsigned char)(tag[3]);
}

static inline int idot(glm::ivec3 a, glm::ivec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}