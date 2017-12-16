#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>
#include <epoxy/gl.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

#include "wiring/wiring_data.h"
#include "physics.h"

struct hw_mesh {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint num_indices;
};


/* TODO: pack this a bit better */
struct vertex {
    float x, y, z;
    uint32_t normal_packed;
    uint32_t uv_packed;

    vertex() : x(0), y(0), z(0) {}

    vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v)
        : x(x), y(y), z(z),
          normal_packed(glm::packSnorm3x10_1x2(glm::vec4(nx, ny, nz, 0))),
          uv_packed(glm::packUnorm2x16(glm::vec2(u,v)))
    {
    }
};

struct sw_mesh {
    vertex *verts;
    unsigned int *indices;
    unsigned int num_vertices;
    unsigned int num_indices;
};

sw_mesh *load_mesh(char const *filename);
hw_mesh *upload_mesh(sw_mesh *mesh);
void draw_mesh(hw_mesh *m);
void free_mesh(hw_mesh *m);
void draw_mesh_instanced(hw_mesh *m, unsigned num_instances);

struct mesh_data {
    std::string mesh{};
    sw_mesh *sw = nullptr;
    hw_mesh *hw = nullptr;
    btTriangleMesh *phys_mesh = nullptr;
    btCollisionShape *phys_shape = nullptr;

    mesh_data() = default;

    explicit mesh_data(const std::string &mesh) : mesh(mesh) {
        sw = load_mesh(mesh.c_str());
    }

    void upload_mesh() {
        assert(sw);

        hw = ::upload_mesh(sw);
    }

    void load_physics() {
        assert(sw);

        build_static_physics_mesh(sw, &phys_mesh, &phys_shape);
    }
};

struct mesh_instance {
    glm::mat4 world_matrix;
    alignas(glm::vec4) int material;
};