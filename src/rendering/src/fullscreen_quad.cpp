#include "fullscreen_quad.h"

// Two triangles covering NDC [-1,1] x [-1,1]
// Using triangle strip with 4 vertices
static const float kQuadVertices[] = {
    // position (x,y)    // texcoord (u,v)
    -1.0f, -1.0f,        0.0f, 0.0f,
     1.0f, -1.0f,        1.0f, 0.0f,
    -1.0f,  1.0f,        0.0f, 1.0f,
     1.0f,  1.0f,        1.0f, 1.0f,
};

FullscreenQuad::FullscreenQuad()
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);

    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // texcoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

FullscreenQuad::~FullscreenQuad()
{
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void FullscreenQuad::draw() const
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
