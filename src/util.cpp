#include <iostream>
#include <string>
#include <GL/gl3w.h>

#include "util.hpp"

/*! \brief Outputs OpenGL errors at the current code position
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

