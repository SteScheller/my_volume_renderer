#include "util.hpp"
#include "geometry.hpp"

#include <array>

#include <GL/gl3w.h>


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
}

util::geometry::Shape::Shape(util::geometry::Shape&& other) :
    m_vertexArrayObject(other.m_vertexArrayObject)
{
    other.m_vertexArrayObject = 0;
}

util::geometry::Shape& util::geometry::Shape::operator=(
        util::geometry::Shape&& other)
{
    if (0 != m_vertexArrayObject)
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    m_vertexArrayObject = other.m_vertexArrayObject;
    other.m_vertexArrayObject = 0;

    return *this;
}

util::geometry::Shape::~Shape()
{
    if (0 != m_vertexArrayObject)
        glDeleteVertexArrays(1, &m_vertexArrayObject);
}

void util::geometry::Shape::bind() const
{
    glBindVertexArray(m_vertexArrayObject);
}

void util::geometry::Shape::unbind() const
{
    glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// CubeFrame
//-----------------------------------------------------------------------------
util::geometry::CubeFrame::CubeFrame(bool oglAvailable)
{
    if (false == oglAvailable)
    {
        m_vertexArrayObject = 0;
        return;
    }

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
    glDeleteBuffers(2, vbo.data());
    glDeleteBuffers(1, &ebo);

}

util::geometry::CubeFrame& util::geometry::CubeFrame::operator=(
        util::geometry::CubeFrame&& other)
{
    Shape::operator=(std::move(other));

    return *this;
}

util::geometry::CubeFrame::~CubeFrame()
{
}

void util::geometry::CubeFrame::draw() const
{
    this->bind();
    glDrawElements(GL_LINES, 2*12, GL_UNSIGNED_INT, 0);
    this->unbind();
}

//-----------------------------------------------------------------------------
// Cube
//-----------------------------------------------------------------------------
util::geometry::Cube::Cube(bool oglAvailable)
{
    if (false == oglAvailable)
    {
        m_vertexArrayObject = 0;
        return;
    }

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
    glDeleteBuffers(2, vbo.data());
    glDeleteBuffers(1, &ebo);
}

util::geometry::Cube& util::geometry::Cube::operator=(
        util::geometry::Cube&& other)
{
    Shape::operator=(std::move(other));

    return *this;
}

util::geometry::Cube::~Cube()
{
}

void util::geometry::Cube::draw() const
{
    this->bind();
    glDrawElements(GL_TRIANGLES, 3 * 2 * 6, GL_UNSIGNED_INT, 0);
    this->unbind();
}

//-----------------------------------------------------------------------------
// Quad
//-----------------------------------------------------------------------------
util::geometry::Quad::Quad(bool oglAvailable)
{
    if (false == oglAvailable)
    {
        m_vertexArrayObject = 0;
        return;
    }

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

util::geometry::Quad& util::geometry::Quad::operator=(
        util::geometry::Quad&& other)
{
    Shape::operator=(std::move(other));

    return *this;
}

util::geometry::Quad::~Quad()
{
}

void util::geometry::Quad::draw() const
{
    this->bind();
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, 0);
    this->unbind();
}

//-----------------------------------------------------------------------------
// Point2D
//-----------------------------------------------------------------------------
util::geometry::Point2D::Point2D(bool oglAvailable)
{
    if (false == oglAvailable)
    {
        m_vertexArrayObject = 0;
        return;
    }

    std::array<float, 2> vertices = {0.f,  0.f};

    GLuint ebo = 0;
    GLuint vbo = 0;

    // create buffers
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    this->bind();

    // vertex coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

    this->unbind();

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

util::geometry::Point2D& util::geometry::Point2D::operator=(
        util::geometry::Point2D&& other)
{
    Shape::operator=(std::move(other));

    return *this;
}

util::geometry::Point2D::~Point2D()
{
}

void util::geometry::Point2D::draw() const
{
    this->bind();
    glDrawArrays(GL_POINTS, 0, 1);
    this->unbind();
}
