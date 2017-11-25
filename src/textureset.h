#pragma once


struct texture_set {
    GLuint texobj;
    int dim;
    int array_size;
    GLenum target;
    unsigned slots;

    texture_set(GLenum target, int dim, int array_size);
    void bind(int texunit);

    unsigned load(const char *filename);
};
