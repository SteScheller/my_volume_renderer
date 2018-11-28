#include <iostream>
#include <string>
#include <GL/gl3w.h>
#include <FreeImage.h>

#include "util.hpp"

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
 *  \brief Creates an OpenGL frame buffer object as alternative render target
 *
 *  \param width            horizontal size in pixel
 *  \param height           vertical size in pixel
 *  \param texIDs           array where the object IDs of the attached
 *                          textures are stored
 *  \param numAttachments   sum of color and attachments that shall be created
 *  \param attachment       array with attachment points (color, depth ...)
 *  \param internalFormat   array with internal format of the attachments
 *  \param format           array with format of the data: GL_RGB,...
 *  \param type             array with data type: GL_UNSIGNED_BYTE, ...
 *  \param filter           array with texture filter: GL_LINEAR or GL_NEAREST
 *
 *  \return The ID of the framebuffer object and the IDs of attached textures
 *          via texIDs.
 */
GLuint util::createFrameBufferObject(
    GLsizei width,
    GLsizei height,
    GLuint texIDs[],
    unsigned int numAttachments,
    GLenum attachment[],
    GLint internalFormat[],
    GLenum format[],
    GLenum datatype[],
    GLint filter[])
{
    GLuint fboID = 0;

    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    for (unsigned int i = 0; i < numAttachments; ++i)
    {
        texIDs[i] = util::create2dTextureObject(
            internalFormat[i],
            format[i],
            datatype[i],
            filter[i],
            width,
            height);

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            attachment[i],
            GL_TEXTURE_2D,
            texIDs[i],
            0);
    }

    printOpenGLError();
    if (!(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)))
        std::cout << "Error: frame buffer object incomplete!\n" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return fboID;
}

/**
 *  \brief Creates a two framebuffer objects for usage as ping pong render
 *         targets
 */
void util::createPingPongFBO(
    GLuint &fbo,
    GLuint texIDs[2],
    unsigned int width,
    unsigned int height)
{
    GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    GLint  internalFormats[2] = { GL_RGBA, GL_RGBA32UI };
    GLenum formats[2] = { GL_RGBA, GL_RGBA_INTEGER };
    GLenum datatypes[2] = { GL_FLOAT, GL_UNSIGNED_INT };
    GLint  filters[2] = { GL_LINEAR, GL_NEAREST };

    fbo = util::createFrameBufferObject(
            width,
            height,
            texIDs,
            2,
            attachments,
            internalFormats,
            formats,
            datatypes,
            filters);
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
        GLuint fbo,
        unsigned int width,
        unsigned int height,
        const char *file,
        FREE_IMAGE_FORMAT type)
{
    GLubyte* pixels = new GLubyte[3 * width * height];

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
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
    FreeImage_Save(type, image, file, 0);

    // Free resources
    FreeImage_Unload(image);
    delete[] pixels;
}



