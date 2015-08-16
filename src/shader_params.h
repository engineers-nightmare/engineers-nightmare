#pragma once

#include <epoxy/gl.h>

/* where T is std140-conforming, and agrees with the shader. */
template<typename T>
struct shader_params
{
    T val;
    GLuint bo;

    shader_params() : bo(0)
    {
        glGenBuffers(1, &bo);
        glBindBuffer(GL_UNIFORM_BUFFER, bo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), NULL, GL_DYNAMIC_DRAW);
    }

    ~shader_params() {
        glDeleteBuffers(1, &bo);
    }

    void upload() {
        /* bind to nonindexed binding point */
        glBindBuffer(GL_UNIFORM_BUFFER, bo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &val);
    }

    void bind(GLuint index) {
        /* bind to proper index of indexed binding point, for use */
        glBindBufferBase(GL_UNIFORM_BUFFER, index, bo);
    }
};


struct per_camera_params {
    glm::mat4 view_proj_matrix;
    glm::mat4 inv_centered_view_proj_matrix;
    float aspect;
};


struct per_object_params {
    glm::mat4 world_matrix;
};


extern shader_params<per_object_params> *per_object;
