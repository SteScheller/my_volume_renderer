#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

#include <GL/gl3w.h>
#include <FreeImage.h>

#include "util.hpp"

//-----------------------------------------------------------------------------
// Framebuffer Class Implementations
//-----------------------------------------------------------------------------
util::FramebufferObject::FramebufferObject() :
    m_ID(0),
    m_textures(std::vector<util::texture::Texture2D>(0)),
    m_attachments(0)
{
}

util::FramebufferObject::FramebufferObject(
        std::vector<util::texture::Texture2D> &&textures,
        const std::vector<GLenum> &attachments) :
    m_ID(0),
    m_textures(std::move(textures)),
    m_attachments(attachments)
{
    if ((m_textures.size() != m_attachments.size()))
        return;

    glGenFramebuffers(1, &m_ID);

    this->bind();

    for (size_t i = 0; i < m_textures.size(); ++i)
    {
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            m_attachments[i],
            GL_TEXTURE_2D,
            m_textures[i].getID(),
            0);

    }

    printOpenGLError();

    if (!(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)))
        std::cerr << "Error: frame buffer object incomplete!" << std::endl;

    this->unbind();
}

util::FramebufferObject::FramebufferObject(util::FramebufferObject&& other) :
    m_ID(other.m_ID),
    m_textures(std::move(other.m_textures)),
    m_attachments(std::move(other.m_attachments))
{
    other.m_ID = 0;
}

util::FramebufferObject& util::FramebufferObject::operator=(
        util::FramebufferObject&& other)
{
    if (0 != m_ID)
        glDeleteFramebuffers(1, &m_ID);

    m_ID = other.m_ID;
    m_textures = std::move(other.m_textures);
    m_attachments = std::move(other.m_attachments);
    other.m_ID = 0;

    return *this;
}

util::FramebufferObject::~FramebufferObject()
{
    if (0 != m_ID)
        glDeleteFramebuffers(1, &m_ID);
}

void util::FramebufferObject::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
    glDrawBuffers(m_attachments.size(), m_attachments.data());
}

void util::FramebufferObject::bindRead(size_t attachmentNumber) const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID);
    glReadBuffer(
        m_attachments[std::min(attachmentNumber, m_attachments.size() - 1)]);
}

void util::FramebufferObject::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//-----------------------------------------------------------------------------
// convenience functions
//-----------------------------------------------------------------------------
/**
 *  \brief Outputs OpenGL errors at the current code position
 *
 *  \param file name of the current code file (__FILE__)
 *  \param line number in the current code file (__LINE__)
 *
 *  \return true if an error occured, false otherwise
 */
bool util::printOglError(const char *file, int line)
{
    GLenum err = glGetError();
    std::string error;
    bool ret = false;

    while(err != GL_NO_ERROR)
    {
        switch(err)
        {
            case GL_INVALID_OPERATION:
                error="INVALID_OPERATION";
                break;

            case GL_INVALID_ENUM:
                error="INVALID_ENUM";
                break;

            case GL_INVALID_VALUE:
                error="INVALID_VALUE";
                break;

            case GL_OUT_OF_MEMORY:
                error="OUT_OF_MEMORY";
                break;

            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error="INVALID_FRAMEBUFFER_OPERATION";
                break;
        }

        std::cerr << "GL_" << error.c_str() << " - " << file << ":" <<
            line << std::endl;
        ret = true;
        err = glGetError();
    }

    return ret;
}
/**
 *  \brief check for existence of file
 */
bool util::checkFile(const std::string& path)
{
    bool result = false;
    std::ifstream f(path.c_str());

    result = f.good();

    return result;
}

/**
 *  \brief Grabs the RGB values from the given FBO and writes them to an image
 *         file.
 *
 *  \param fbo object from which the pixel shall be read
 *  \param width horizontal size of the fbo object in pixel
 *  \param height vertical size of the fbo object in pixel
 *  \param file name and path of the target bmp screenshot file
 *  \param type FreeImage Image type (FIF_BMP, FIF_TIFF, ...)
 */
void util::makeScreenshot(
        const FramebufferObject &fbo,
        unsigned int width,
        unsigned int height,
        const std::string &file,
        FREE_IMAGE_FORMAT type)
{
    GLubyte* pixels = new GLubyte[3 * width * height];

    fbo.bind();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels);

    // Convert to FreeImage format & save to file
    FIBITMAP* image = FreeImage_ConvertFromRawBits(
        pixels,
        width,
        height,
        3 * width,
        24,
        0x0000FF,
        0x00FF00,
        0xFF0000,
        false);
    FreeImage_Save(type, image, file.c_str(), 0);

    // Free resources
    FreeImage_Unload(image);
    delete[] pixels;
}

