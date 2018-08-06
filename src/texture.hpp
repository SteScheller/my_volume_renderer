#ifndef __TEXTURE_HPP
#define __TEXTURE_HPP

#include <GL/gl3w.h>

namespace util
{
    /**
     * @brief creates a 3D texture filled with the given values
     * @param buf array containing the volume data in linear order
     * @param internalFormat OGL Define that determines the number of color componenents and their type
     * @param res_x Number of values in the first dimension
     * @param res_y Number of values in the second dimension
     * @param res_z Number of values in the third dimension
     */
    GLuint create3dTexFromScalar(
        const GLvoid *buf,
        GLint internalFormat,
        GLsizei res_x,
        GLsizei res_y,
        GLsizei res_z)
    {
        GLuint volumeTex;

        glGenTextures(1, &volumeTex);
        glBindTexture(GL_TEXTURE_3D, volumeTex);
        glTexImage3D(
            GL_TEXTURE_3D,
            0,
            internalFormat,
            res_x,
            res_y,
            res_z,
            0,
            GL_RED,
            GL_FLOAT,
            buf);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        float borderColor[] = { 0.f, 0.f, 0.f, 1.f };
        glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor);

        return volumeTex;
    }
}

#endif
