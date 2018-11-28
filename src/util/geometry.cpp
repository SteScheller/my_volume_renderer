#include <array>

#include <GL/gl3w.h>

#include "util.hpp"

//-----------------------------------------------------------------------------
// convenience functions
//-----------------------------------------------------------------------------
GLuint util::createFrameVAO(
        const float vertices[4 * 8],
        const unsigned int indices[2 * 12],
        const float texCoords[3 * 8])
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Utility Geometry Class Implementations
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Shape Basis Class
//-----------------------------------------------------------------------------
util::geometry::Shape::Shape()
    : m_vertexArrayObject(0)
{
    GLuint ebo = 0;
    GLuint vbo[2] = {0, 0};

    // create buffers
    glGenVertexArrays(1, &frameVAO);
    glGenBuffers(2, vbo);
    glGenBuffers(1, &ebo);

    this->bind();

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(
        GL_ARRAY_BUFFER, 32 * sizeof(float), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        24 * sizeof(unsigned int),
        indices,
        GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(
        GL_ARRAY_BUFFER, 24 * sizeof(float), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(1);

    // unbind vao and delete buffers that are not need anymore
    glBindVertexArray(0);
    glDeleteBuffers(2, vbo);
    glDeleteBuffers(1, &ebo);

    return frameVAO;
}

util::geometry::Shape::~Shape()
{
    glDeleteVertexArrays(m_vertexArrayObject);
}

util::geometry::Shape::bind()
{
    glBindVertexArray(m_vertexArrayObject);
}

util::geometry::Shape::unbind()
{
    glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// CubeFrame
//-----------------------------------------------------------------------------
util::geometry::CubeFrame::CubeFrame()
{
    std::array<float, 4 * 8> vertices = {
        -0.5f,  -0.5f,  -0.5f,  1.f,
        0.5f,   -0.5f,  -0.5f,  1.f,
        0.5f,   0.5f,   -0.5f,  1.f,
        -0.5f,  0.5f,   -0.5f,  1.f,
        -0.5f,  -0.5f,  0.5f,   1.f,
        0.5f,   -0.5f,  0.5f,   1.f,
        0.5f,   0.5f,   0.5f,   1.f,
        -0.5f,  0.5f,   0.5f,   1.f
    };

    std::array<float, 3 * 8> coordinates = {
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 1.f,
        0.f, 1.f, 1.f,
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 1.f, 0.f
    };

    std::array<unsigned int, 2 * 12> indices = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        4, 5,
        5, 6,
        6, 7,
        7, 4,
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    // TODO: continue here and check cube again!!!
    //
    //
    GLuint ebo = 0;
    GLuint vbo[2] = {0, 0};

    // create buffers
    glGenVertexArrays(1, &frameVAO);
    glGenBuffers(2, vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(frameVAO);

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(
        GL_ARRAY_BUFFER, 32 * sizeof(float), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        24 * sizeof(unsigned int),
        indices,
        GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(
        GL_ARRAY_BUFFER, 24 * sizeof(float), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(1);

    // unbind vao and delete buffers that are not need anymore
    glBindVertexArray(0);
    glDeleteBuffers(2, vbo);
    glDeleteBuffers(1, &ebo);

 }

util::geometry::CubeFrame::~CubeFrame()
{
}

util::geometry::CubeFrame::draw()
{
    this->bind();
    glDrawElements(GL_LINES, 2*12, GL_UNSIGNED_INT, 0);
    this->unbind();
}

//-----------------------------------------------------------------------------
// Cube
//-----------------------------------------------------------------------------
util::geometry::Cube::Cube()
{
    std::array<float, 4 * 8> vertices = {
        -0.5f,  -0.5f,  -0.5f,  1.f,
        0.5f,   -0.5f,  -0.5f,  1.f,
        0.5f,   0.5f,   -0.5f,  1.f,
        -0.5f,  0.5f,   -0.5f,  1.f,
        -0.5f,  -0.5f,  0.5f,   1.f,
        0.5f,   -0.5f,  0.5f,   1.f,
        0.5f,   0.5f,   0.5f,   1.f,
        -0.5f,  0.5f,   0.5f,   1.f
    };

    std::array<float, 3 * 8> coordinates = {
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 1.f,
        0.f, 1.f, 1.f,
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 1.f, 0.f
    };

    std::array<unsigned int, 3 * 2 * 6> indices = {
        2, 1, 0,
        0, 3, 2,
        6, 5, 1,
        1, 2, 6,
        7, 4, 5,
        5, 6, 7,
        3, 0, 4,
        4, 7, 3,
        3, 7, 6,
        6, 2, 3,
        4, 0, 1,
        1, 5, 4
    };

    GLuint ebo = 0;
    std::array<GLuint, 2> vbo = {0, 0};

    // create buffers
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(2, vbo.data());
    glGenBuffers(1, &ebo);

    this->bind();

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,
        4,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        (void*) 0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(
        GL_ARRAY_BUFFER,
        coordinates.size() * sizeof(float),
        coordinates.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*) 0);
    glEnableVertexAttribArray(1);

    this->unbind();
    glBindVertexArray(0);
    glDeleteBuffers(2, vbo);
    glDeleteBuffers(1, &ebo);
}

util::geometry::Cube::~Cube()
{
}

util::geometry::Cube::draw()
{
    this->bind();
    glDrawElements(GL_TRIANGLES, 3 * 2 * 6, GL_UNSIGNED_INT, 0);
    this->unbind();
}
//-----------------------------------------------------------------------------
// Quad
//-----------------------------------------------------------------------------
util::geometry::Quad::Quad()
{
    std::array<float, 2 * 4> vertices = {
        -0.5f,  -0.5f,
        0.5f,   -0.5f,
        0.5f,   0.5f,
        -0.5f,  0.5f
    };

    std::array<float, 2 * 4> coordinates = {
        0.f,    0.f,
        1.f,    0.f,
        1.f,    1.f,
        0.f,    1.f
    };

    std::array<unsigned int, 4> indices = { 0, 1, 2, 3 };

    GLuint ebo = 0;
    std::array<GLuint, 2> vbo = {0, 0};

    // create buffers
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(2, vbo.data());
    glGenBuffers(1, &ebo);

    this->bind();

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*) 0);
    glEnableVertexAttribArray(0);

    // vertex indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    // texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(
        GL_ARRAY_BUFFER,
        coordinates.size() * sizeof(float),
        coordinates.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*) 0);
    glEnableVertexAttribArray(1);

    this->unbind();
    glDeleteBuffers(2, vbo.data());
    glDeleteBuffers(1, &ebo);
}

util::geometry::Quad::~Quad()
{
}

util::geometry::Quad::draw()
{
    this->bind();
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, 0);
    this->unbind();
}
