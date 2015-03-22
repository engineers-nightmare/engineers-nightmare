#include <err.h>
#include <epoxy/gl.h>

#include "shader.h"
#include "blob.h"

static GLuint
load_stage(GLenum stage, char const *filename)
{
    blob content(filename);
    GLuint shader = glCreateShader(stage);
    glObjectLabel(GL_SHADER, shader, -1, filename);
    GLint len = content.len;
    glShaderSource(shader, 1, (GLchar const **) &content.data, &len);
    glCompileShader(shader);
}

GLuint load_shader(char const *vs, char const *fs)
{
    GLuint prog = glCreateProgram();

    GLuint vs_obj = load_stage(GL_VERTEX_SHADER, vs);
    glAttachShader(prog, vs_obj);

    GLuint fs_obj = load_stage(GL_FRAGMENT_SHADER, fs);
    glAttachShader(prog, fs_obj);

    glLinkProgram(prog);

    glDetachShader(prog, vs_obj);
    glDeleteShader(vs_obj);

    glDetachShader(prog, fs_obj);
    glDeleteShader(fs_obj);

    return prog;
}
