#pragma once

struct hw_mesh {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint num_indices;
};


/* TODO: pack this a bit better */
struct vertex {
    float x, y, z;
    vertex() : x(0), y(0), z(0) {}
    vertex(float x, float y, float z) : x(x), y(y), z(z) {}
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
