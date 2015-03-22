#pragma once

struct mesh {
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


mesh *load_mesh(char const *filename);
void draw_mesh(mesh *m);
