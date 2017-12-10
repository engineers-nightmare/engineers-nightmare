//
// Created by caleb on 12/5/17.
//

// ========================================================
// Debug Draw RenderInterface for Core OpenGL:
// ========================================================

#include <cassert>
#include <epoxy/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "debugdraw.h"

static inline void errorF(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);

    // Default newline and flush (like std::endl)
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

static void compileShader(const GLuint shader)
{
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLchar strInfoLog[1024] = {0};
        glGetShaderInfoLog(shader, sizeof(strInfoLog) - 1, nullptr, strInfoLog);
        errorF("\n>>> Shader compiler errors:\n%s", strInfoLog);
    }
}

static void linkProgram(const GLuint program)
{
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLchar strInfoLog[1024] = {0};
        glGetProgramInfoLog(program, sizeof(strInfoLog) - 1, nullptr, strInfoLog);
        errorF("\n>>> Program linker errors:\n%s", strInfoLog);
    }
}

//
// dd::RenderInterface overrides:
//

void DDRenderInterfaceCoreGL::drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled)
{
    assert(points != nullptr);
    assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

    glBindVertexArray(linePointVAO);
    glUseProgram(linePointProgram);

    glUniformMatrix4fv(linePointProgram_MvpMatrixLocation,
                       1, GL_FALSE, glm::value_ptr(mvpMatrix));

    if (depthEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    // NOTE: Could also use glBufferData to take advantage of the buffer orphaning trick...
    glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), points);

    // Issue the draw call:
    glDrawArrays(GL_POINTS, 0, count);

    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DDRenderInterfaceCoreGL::drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled)
{
    assert(lines != nullptr);
    assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

    glBindVertexArray(linePointVAO);
    glUseProgram(linePointProgram);

    glUniformMatrix4fv(linePointProgram_MvpMatrixLocation,
                       1, GL_FALSE, glm::value_ptr(mvpMatrix));

    if (depthEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    // NOTE: Could also use glBufferData to take advantage of the buffer orphaning trick...
    glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), lines);

    // Issue the draw call:
    glDrawArrays(GL_LINES, 0, count);

    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

    // These two can also be implemented to perform GL render
    // state setup/cleanup, but we don't use them in this sample.
    //void beginDraw() override { }
    //void endDraw()   override { }

    //
    // Local methods:
    //

DDRenderInterfaceCoreGL::DDRenderInterfaceCoreGL()
    : mvpMatrix(glm::mat4())
    , linePointProgram(0)
    , linePointProgram_MvpMatrixLocation(-1)
    , textProgram(0)
    , textProgram_GlyphTextureLocation(-1)
    , textProgram_ScreenDimensions(-1)
    , linePointVAO(0)
    , linePointVBO(0)
    , textVAO(0)
    , textVBO(0)
{
    std::printf("\n");
    std::printf("GL_VENDOR    : %s\n",   glGetString(GL_VENDOR));
    std::printf("GL_RENDERER  : %s\n",   glGetString(GL_RENDERER));
    std::printf("GL_VERSION   : %s\n",   glGetString(GL_VERSION));
    std::printf("GLSL_VERSION : %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::printf("DDRenderInterfaceCoreGL initializing ...\n");

    // Default OpenGL states:
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // This has to be enabled since the point drawing shader will use gl_PointSize.
    glEnable(GL_PROGRAM_POINT_SIZE);

    setupShaderPrograms();
    setupVertexBuffers();

    std::printf("DDRenderInterfaceCoreGL ready!\n\n");
}

DDRenderInterfaceCoreGL::~DDRenderInterfaceCoreGL()
{
    glDeleteProgram(linePointProgram);
    glDeleteProgram(textProgram);

    glDeleteVertexArrays(1, &linePointVAO);
    glDeleteBuffers(1, &linePointVBO);

    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
}

void DDRenderInterfaceCoreGL::setupShaderPrograms()
{
    std::printf("> DDRenderInterfaceCoreGL::setupShaderPrograms()\n");

    //
    // Line/point drawing shader:
    //
    {
        GLuint linePointVS = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(linePointVS, 1, &linePointVertShaderSrc, nullptr);
        compileShader(linePointVS);

        GLint linePointFS = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(linePointFS, 1, &linePointFragShaderSrc, nullptr);
        compileShader(linePointFS);

        linePointProgram = glCreateProgram();
        glAttachShader(linePointProgram, linePointVS);
        glAttachShader(linePointProgram, linePointFS);

        glBindAttribLocation(linePointProgram, 0, "in_Position");
        glBindAttribLocation(linePointProgram, 1, "in_ColorPointSize");
        linkProgram(linePointProgram);

        linePointProgram_MvpMatrixLocation = glGetUniformLocation(linePointProgram, "u_MvpMatrix");
        if (linePointProgram_MvpMatrixLocation < 0)
        {
            errorF("Unable to get u_MvpMatrix uniform location!");
        }
    }

    //
    // Text rendering shader:
    //
    {
        GLuint textVS = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(textVS, 1, &textVertShaderSrc, nullptr);
        compileShader(textVS);

        GLint textFS = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(textFS, 1, &textFragShaderSrc, nullptr);
        compileShader(textFS);

        textProgram = glCreateProgram();
        glAttachShader(textProgram, textVS);
        glAttachShader(textProgram, textFS);

        glBindAttribLocation(textProgram, 0, "in_Position");
        glBindAttribLocation(textProgram, 1, "in_TexCoords");
        glBindAttribLocation(textProgram, 2, "in_Color");
        linkProgram(textProgram);

        textProgram_GlyphTextureLocation = glGetUniformLocation(textProgram, "u_glyphTexture");
        if (textProgram_GlyphTextureLocation < 0)
        {
            errorF("Unable to get u_glyphTexture uniform location!");
        }

        textProgram_ScreenDimensions = glGetUniformLocation(textProgram, "u_screenDimensions");
        if (textProgram_ScreenDimensions < 0)
        {
            errorF("Unable to get u_screenDimensions uniform location!");
        }
    }
}

void DDRenderInterfaceCoreGL::setupVertexBuffers()
{
    std::printf("> DDRenderInterfaceCoreGL::setupVertexBuffers()\n");

    //
    // Lines/points vertex buffer:
    //
    {
        glGenVertexArrays(1, &linePointVAO);
        glGenBuffers(1, &linePointVBO);

        glBindVertexArray(linePointVAO);
        glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);

        // RenderInterface will never be called with a batch larger than
        // DEBUG_DRAW_VERTEX_BUFFER_SIZE vertexes, so we can allocate the same amount here.
        glBufferData(GL_ARRAY_BUFFER, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), nullptr, GL_STREAM_DRAW);

        // Set the vertex format expected by 3D points and lines:
        std::size_t offset = 0;

        glEnableVertexAttribArray(0); // in_Position (vec3)
        glVertexAttribPointer(
            /* index     = */ 0,
            /* size      = */ 3,
            /* type      = */ GL_FLOAT,
            /* normalize = */ GL_FALSE,
            /* stride    = */ sizeof(dd::DrawVertex),
            /* offset    = */ reinterpret_cast<void *>(offset));
        offset += sizeof(float) * 3;

        glEnableVertexAttribArray(1); // in_ColorPointSize (vec4)
        glVertexAttribPointer(
            /* index     = */ 1,
            /* size      = */ 4,
            /* type      = */ GL_FLOAT,
            /* normalize = */ GL_FALSE,
            /* stride    = */ sizeof(dd::DrawVertex),
            /* offset    = */ reinterpret_cast<void *>(offset));

        // VAOs can be a pain in the neck if left enabled...
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //
    // Text rendering vertex buffer:
    //
    {
        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);

        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);

        // NOTE: A more optimized implementation might consider combining
        // both the lines/points and text buffers to save some memory!
        glBufferData(GL_ARRAY_BUFFER, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), nullptr, GL_STREAM_DRAW);

        // Set the vertex format expected by the 2D text:
        std::size_t offset = 0;

        glEnableVertexAttribArray(0); // in_Position (vec2)
        glVertexAttribPointer(
            /* index     = */ 0,
            /* size      = */ 2,
            /* type      = */ GL_FLOAT,
            /* normalize = */ GL_FALSE,
            /* stride    = */ sizeof(dd::DrawVertex),
            /* offset    = */ reinterpret_cast<void *>(offset));
        offset += sizeof(float) * 2;

        glEnableVertexAttribArray(1); // in_TexCoords (vec2)
        glVertexAttribPointer(
            /* index     = */ 1,
            /* size      = */ 2,
            /* type      = */ GL_FLOAT,
            /* normalize = */ GL_FALSE,
            /* stride    = */ sizeof(dd::DrawVertex),
            /* offset    = */ reinterpret_cast<void *>(offset));
        offset += sizeof(float) * 2;

        glEnableVertexAttribArray(2); // in_Color (vec4)
        glVertexAttribPointer(
            /* index     = */ 2,
            /* size      = */ 4,
            /* type      = */ GL_FLOAT,
            /* normalize = */ GL_FALSE,
            /* stride    = */ sizeof(dd::DrawVertex),
            /* offset    = */ reinterpret_cast<void *>(offset));

        // Ditto.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// ========================================================
// Minimal shaders we need for the debug primitives:
// ========================================================

const char * DDRenderInterfaceCoreGL::linePointVertShaderSrc = R"(
    #version 150

    in vec3 in_Position;
    in vec4 in_ColorPointSize;

    out vec4 v_Color;
    uniform mat4 u_MvpMatrix;

    void main()
    {
        gl_Position  = u_MvpMatrix * vec4(in_Position, 1.0);
        gl_PointSize = in_ColorPointSize.w;
        v_Color      = vec4(in_ColorPointSize.xyz, 1.0);
    }
)";

const char * DDRenderInterfaceCoreGL::linePointFragShaderSrc = R"(
    #version 150
    
    in  vec4 v_Color;
    out vec4 out_FragColor;
    
    void main()
    {
        out_FragColor = v_Color;
    }
)";

const char * DDRenderInterfaceCoreGL::textVertShaderSrc = R"(
    #version 150
    
    in vec2 in_Position;
    in vec2 in_TexCoords;
    in vec3 in_Color;
    
    uniform vec2 u_screenDimensions;
    
    out vec2 v_TexCoords;
    out vec4 v_Color;
    
    void main()
    {
        // Map to normalized clip coordinates:
        float x = ((2.0 * (in_Position.x - 0.5)) / u_screenDimensions.x) - 1.0;
        float y = 1.0 - ((2.0 * (in_Position.y - 0.5)) / u_screenDimensions.y);
    
        gl_Position = vec4(x, y, 0.0, 1.0);
        v_TexCoords = in_TexCoords;
        v_Color     = vec4(in_Color, 1.0);
    }
)";

const char * DDRenderInterfaceCoreGL::textFragShaderSrc = R"(
    #version 150
    
    in vec2 v_TexCoords;
    in vec4 v_Color;
    
    uniform sampler2D u_glyphTexture;
    out vec4 out_FragColor;
    
    void main()
    {
        out_FragColor = v_Color;
        out_FragColor.a = texture(u_glyphTexture, v_TexCoords).r;
    }
)";