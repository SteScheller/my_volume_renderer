#include <random>

#include <GL/gl3w.h>

#include "util.hpp"
#include "texture.hpp"

//-----------------------------------------------------------------------------
// texture class implementations
//-----------------------------------------------------------------------------
util::texture::Texture::Texture() :
    m_ID(0)
{
}

util::texture::Texture::Texture(util::texture::Texture&& other) :
    m_ID(other.m_ID)
{
    other.m_ID = 0;
}

util::texture::Texture& util::texture::Texture::operator=(
        util::texture::Texture&& other)
{
    if (0 != m_ID)
        glDeleteTextures(1, &m_ID);
    m_ID = other.m_ID;
    other.m_ID = 0;

    return *this;
}

util::texture::Texture::~Texture()
{
    if (0 != m_ID)
        glDeleteTextures(1, &m_ID);
}

//-----------------------------------------------------------------------------
util::texture::Texture2D::Texture2D ()
{
}

/**
 * \brief Creates a 2D texture object which also can be used as render target
 * \param internalFormat internal format of the texture
 * \param format         format of the data: GL_RGB,...
 * \param level          level of detail number: 0 for base level
 * \param type           data type: GL_UNSIGNED_BYTE, GL_FLOAT,...
 * \param filter         texture filter: GL_LINEAR or GL_NEAREST
 * \param wrap           texture wrap: GL_CLAMP_TO_EDGE, ...
 * \param width          horizontal resolution
 * \param height         vertical resolution
 * \param buf            array containing data for initializing the texture
 *
 * \return OpenGL ID of the texture Object
 */
util::texture::Texture2D::Texture2D(
    GLenum internalFormat,
    GLenum format,
    GLint level,
    GLenum type,
    GLint filter,
    GLint wrap,
    GLsizei width,
    GLsizei height,
    const GLvoid * data,
    const std::array<float, 4> &borderColor)
{
    glGenTextures(1, &m_ID);

    this->bind();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        level,
        internalFormat,
        width,
        height,
        0,
        format,
        type,
        data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameterfv(
        GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());

    this->unbind();
}

util::texture::Texture2D::Texture2D(
        util::texture::Texture2D&& other) :
    Texture(std::move(other))
{
}

util::texture::Texture2D& util::texture::Texture2D::operator=(
        util::texture::Texture2D&& other)
{
    Texture::operator=(std::move(other));

    return *this;
}

util::texture::Texture2D::~Texture2D()
{
}

void util::texture::Texture2D::bind() const
{
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

void util::texture::Texture2D::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

//-----------------------------------------------------------------------------
util::texture::Texture3D::Texture3D ()
{
}

/**
 * \brief Creates a 2D texture object which also can be used as render target
 * \param internalFormat internal format of the texture
 * \param format         format of the data: GL_RGB,...
 * \param level          level of detail number: 0 for base level
 * \param type           data type: GL_UNSIGNED_BYTE, GL_FLOAT,...
 * \param filter         texture filter: GL_LINEAR or GL_NEAREST
 * \param wrap           texture wrap: GL_CLAMP_TO_EDGE, ...
 * \param width          horizontal resolution
 * \param height         vertical resolution
 * \param depth          z dimension resolution
 * \param data           array containing data for initializing the texture
 *
 * \return OpenGL ID of the texture Object
 */
util::texture::Texture3D::Texture3D(
    GLenum internalFormat,
    GLenum format,
    GLint level,
    GLenum type,
    GLint filter,
    GLint wrap,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    const GLvoid * data,
    const std::array<float, 4> &borderColor)
{
    glGenTextures(1, &m_ID);

    this->bind();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(
        GL_TEXTURE_3D,
        level,
        internalFormat,
        width,
        height,
        depth,
        0,
        format,
        type,
        data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);
    glTexParameterfv(
        GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor.data());

    this->unbind();
}

util::texture::Texture3D::Texture3D(
        util::texture::Texture3D&& other) :
    Texture(std::move(other))
{
}

util::texture::Texture3D& util::texture::Texture3D::operator=(
        util::texture::Texture3D&& other)
{
    Texture::operator=(std::move(other));

    return *this;
}

util::texture::Texture3D::~Texture3D()
{
}

void util::texture::Texture3D::bind() const
{
    glBindTexture(GL_TEXTURE_3D, m_ID);
}

void util::texture::Texture3D::unbind() const
{
    glBindTexture(GL_TEXTURE_3D, 0);
}
//-----------------------------------------------------------------------------
// convenience functions
//-----------------------------------------------------------------------------
/**
 * \brief intializes a texture use with the HybridTaus random generator
 *
 * \param width            texture width
 * \param height           texture height
 */
util::texture::Texture2D util::texture::create2dHybridTausTexture(
        GLsizei width,
        GLsizei height)
{
    uint32_t *buf = new uint32_t[4 * width * height];
    uint32_t r1 = 0, r2 = 0, r3 = 0, r4 = 0;

    std::random_device rd;  // for getting one non-deterministic random number
                            // and seeding the mersenne twister
    std::mt19937 mt(rd());  // for actually sampling subsequent (pseudo-)
                            // random numbers with good performance
    std::uniform_int_distribution<uint32_t> distribution(128, 1023);

    r1 = distribution(mt);
    r2 = distribution(mt);
    r3 = distribution(mt);
    r4 = distribution(mt);
    for (size_t i = 0; i < static_cast<size_t>(width * height); ++i)
    {
        buf[i * 4] = r1 + static_cast<uint32_t>((1 + i) << 10);
        buf[i * 4 + 1] = r2 + static_cast<uint32_t>((1 + i) << 10);
        buf[i * 4 + 2] = r3 + static_cast<uint32_t>((1 + i) << 10);
        buf[i * 4 + 3] = r4 + static_cast<uint32_t>((1 + i) << 10);
    }

   util::texture::Texture2D htTex(
        GL_RGBA32UI,
        GL_RGBA_INTEGER,
        0,
        GL_UNSIGNED_INT,
        GL_NEAREST,
        GL_REPEAT,
        width,
        height,
        buf);

    delete[] buf;

    return htTex;
}

