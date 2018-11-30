#include <iostream>
#include <string>
#include <vector>
#include <GL/gl3w.h>

#include <FreeImage.h>

#include "util.hpp"

//-----------------------------------------------------------------------------
// Framebuffer Class Implementations
//-----------------------------------------------------------------------------
util::FrameBufferObject::FrameBufferObject() :
    m_ID(0),
    m_textures()
{
}

util::FramebufferObject::FrameBufferObject(
        const std::vector<util::texture::Texture2D&> &textures,
        const std::vector<GLenum> &attachments) :
    FramebufferObject()
{
    if ((textures.size() < 0) || (textures.size() != attachments.size()))
            return;

    glGenFramebuffers(1, &m_ID);
    m_textures = textures;

    this->bind()

    for (size_t i = 0; i < textures.size(); ++i)
    {
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            attachments[i],
            GL_TEXTURE_2D,
            texures[i].getID(),
            0);

    }

    printOpenGLError();

    if (!(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)))
        std::cerr << "Error: frame buffer object incomplete!\n" << std::endl;

    this->unbind();
}

util::FrameBufferObject::FrameBufferObject(util::FrameBufferObject&& other)
{
    this->m_ID = other.m_ID;
    this->m_textures = std::move(other.m_textures);
    other.m_ID = 0;
}

util::FrameBufferObject& util::FramebufferObject::operator=(
        util::FrameBufferObject&& other)
{
    glDeleteFramebuffers(1, &(this->m_ID));
    this->m_ID = other.m_ID;
    this->m_textures = std::move(other.m_textures);
    other.m_ID = 0;

    return *this;
}

util::FrameBufferObject::~FramebufferObject()
{
    glDeleteFramebuffers(1, &m_ID);
}

util::FrameBufferObject::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
}

util::FrameBufferObject::bindRead() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID);
}

util::FrameBufferObject::unbind() const
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
        FramebufferObject fbo,
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



