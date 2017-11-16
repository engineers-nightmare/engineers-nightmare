#pragma once


struct texture_set {
    GLuint texobj;
    int dim;
    int array_size;
    GLenum target;

    texture_set(GLenum target, int dim, int array_size);
    void bind(int texunit);
    void load(int slot, char const *filename);
    void load_empty(int slot);
};
