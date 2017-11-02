#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif

#include <epoxy/gl.h>
#include <stdio.h>
#include <string>

#include "shader.h"
#include "blob.h"

static GLuint
load_stage(GLenum stage, char const *filename)
{
    blob content(filename);
    GLuint shader = glCreateShader(stage);
    glObjectLabel(GL_SHADER, shader, -1, filename);
    GLint len = (GLint)content.len;
    glShaderSource(shader, 1, (GLchar const **) &content.data, &len);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string error;
        error.resize((unsigned)len);
        glGetProgramInfoLog(shader, 255, &len, &error[0]);
        printf("Shader %s failed with --\n  %s\n", filename, error.c_str());
    }

    return shader;
}

GLuint load_shader(char const *vs, char const *fs)
{
    GLuint prog = glCreateProgram();

    GLuint vs_obj = load_stage(GL_VERTEX_SHADER, vs);
    glAttachShader(prog, vs_obj);

    GLuint fs_obj = load_stage(GL_FRAGMENT_SHADER, fs);
    glAttachShader(prog, fs_obj);

    glLinkProgram(prog);

    GLint linked;
    glGetShaderiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint len;
        glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string error;
        error.resize((unsigned)len);
        glGetProgramInfoLog(prog, 255, &len, &error[0]);
        printf("Program of %s : %s failed with --\n  %s\n", vs, fs, error.c_str());
    }

    glDetachShader(prog, vs_obj);
    glDeleteShader(vs_obj);

    glDetachShader(prog, fs_obj);
    glDeleteShader(fs_obj);

    /* NOTE: you'd normally want to check that the shader compile & link actually succeeded,
     * but we're going to rely on KHR_debug as the feedback channel there. As a nice side effect,
     * we also get to take full advantage of deferred shader compilation if the GL does it
     * (assuming glLinkProgram doesn't force completion of the individual shader compiles)
     *
     * This is all somewhat implementation-specific guesswork... but it's also free.
     */

    printf("Loaded shader vs:%s fs:%s\n", vs, fs);

    return prog;
}
