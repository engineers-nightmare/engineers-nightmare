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
#include "../shader.h"

#define DEBUG_DRAW_IMPLEMENTATION
#define DEBUG_DRAW_CXX11_SUPPORTED 1
#include "debugdraw.h"


void DDRenderInterfaceCoreGL::drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled)
{
    assert(points != nullptr);
    assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

    glBindVertexArray(linePointVAO);
    glUseProgram(linePointProgram);

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
}


void DDRenderInterfaceCoreGL::drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled)
{
    assert(lines != nullptr);
    assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

    glBindVertexArray(linePointVAO);
    glUseProgram(linePointProgram);

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
}


DDRenderInterfaceCoreGL::DDRenderInterfaceCoreGL()
    : linePointProgram(0)
    , linePointVAO(0)
    , linePointVBO(0)

{
    // Default OpenGL states:
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // This has to be enabled since the point drawing shader will use gl_PointSize.
    glEnable(GL_PROGRAM_POINT_SIZE);

    setupShaderPrograms();
    setupVertexBuffers();
}


DDRenderInterfaceCoreGL::~DDRenderInterfaceCoreGL()
{
    glDeleteProgram(linePointProgram);
    glDeleteVertexArrays(1, &linePointVAO);
    glDeleteBuffers(1, &linePointVBO);
}


void DDRenderInterfaceCoreGL::setupShaderPrograms()
{
    linePointProgram = load_shader("shaders/debug.vert", "shaders/debug.frag");
}


void DDRenderInterfaceCoreGL::setupVertexBuffers()
{
    glGenVertexArrays(1, &linePointVAO);
    glGenBuffers(1, &linePointVBO);

    glBindVertexArray(linePointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);

    // RenderInterface will never be called with a batch larger than
    // DEBUG_DRAW_VERTEX_BUFFER_SIZE vertexes, so we can allocate the same amount here.
    glBufferData(GL_ARRAY_BUFFER, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0); // in_Position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(dd::DrawVertex), (GLvoid const *)offsetof(dd::DrawVertex, line.x));
    glEnableVertexAttribArray(1); // in_ColorPointSize (vec4)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(dd::DrawVertex), (GLvoid const *)offsetof(dd::DrawVertex, line.r));
}
