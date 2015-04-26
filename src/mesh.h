#pragma once

struct hw_mesh {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint num_indices;
};


/* TODO: pack this a bit better */
struct vertex {
    float x, y, z, nx, ny, nz;
    int mat;
    vertex() : x(0), y(0), z(0), mat(0) {}
    vertex(float x, float y, float z, float nx, float ny, float nz, int mat)
        : x(x), y(y), z(z), nx(nx), ny(ny), nz(nz), mat(mat) {}
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
