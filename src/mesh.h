#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>

#include <epoxy/gl.h>

struct hw_mesh {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint num_indices;
};


/* TODO: pack this a bit better */
/* TODO: use packed normals on drivers that will accept them reliably. This
 * provokes a crash on AMD/windows.
 */
struct vertex {
    float x, y, z;
    float nx, ny, nz;
    //uint32_t normal_packed;
    uint32_t mat;

    vertex() : x(0), y(0), z(0), mat(0) {}

    vertex(float x, float y, float z, float nx, float ny, float nz, int mat)
        : x(x), y(y), z(z),
          nx(nx), ny(ny), nz(nz),
          //normal_packed(glm::packSnorm3x10_1x2(glm::vec4(nx, ny, nz, 0))),
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
void draw_mesh_instanced(hw_mesh *m, unsigned num_instances);
