#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>

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
    uint16_t mat;

    vertex() : x(0), y(0), z(0), mat(0) {}

    vertex(float x, float y, float z, float nx, float ny, float nz, int mat)
        : x(x), y(y), z(z),
          normal_packed(glm::packSnorm3x10_1x2(glm::vec4(nx, ny, nz, 0))),
          mat(mat)
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
void set_mesh_material(sw_mesh *m, int material);
void draw_mesh(hw_mesh *m);
void free_mesh(hw_mesh *m);
