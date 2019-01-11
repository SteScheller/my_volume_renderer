#pragma once

#include <array>

#include <GL/gl3w.h>

#include "util.hpp"

namespace util
{
    namespace texture
    {
        //---------------------------------------------------------------------
        // Texture classes
        //---------------------------------------------------------------------
        class Texture
        {
            public:
            Texture();
            Texture(const Texture& other) = delete;
            Texture(Texture&& other);
            Texture& operator=(const Texture& other) = delete;
            Texture& operator=(Texture&& other);

            virtual ~Texture();

            GLuint getID() const {return m_ID;}

            virtual void unbind() const = 0;
            virtual void bind() const = 0;

            protected:
            GLuint m_ID;
        };

        class Texture2D : public Texture
        {
            public:
            Texture2D();
            Texture2D(
                GLenum internalFormat,
                GLenum format,
                GLint level,
                GLenum type,
                GLint filter,
                GLint wrap,
                GLsizei width,
                GLsizei height,
                const GLvoid * data = static_cast<const GLvoid*>(nullptr),
                const std::array<float, 4> &borderColor =
                    {0.f, 0.f, 0.f, 1.f} );
            Texture2D(const Texture2D& other) = delete;
            Texture2D(Texture2D&& other);
            Texture2D& operator=(const Texture2D& other) = delete;
            Texture2D& operator=(Texture2D&& other);

            ~Texture2D();

            void unbind() const;
            void bind() const;
        };

        class Texture3D : public Texture
        {
            public:
            Texture3D();
            Texture3D(
                GLenum internalFormat,
                GLenum format,
                GLint level,
                GLenum type,
                GLint filter,
                GLint wrap,
                GLsizei width,
                GLsizei height,
                GLsizei depth,
                const GLvoid * data = static_cast<const GLvoid*>(nullptr),
                const std::array<float, 4> &borderColor =
                    {0.f, 0.f, 0.f, 1.f} );
            Texture3D(const Texture3D& other) = delete;
            Texture3D(Texture3D&& other);
            Texture3D& operator=(const Texture3D& other) = delete;
            Texture3D& operator=(Texture3D&& other);

            ~Texture3D();

            void unbind() const;
            void bind() const;
        };
        //---------------------------------------------------------------------
        // Convenience Functions
        //---------------------------------------------------------------------
        Texture2D create2dHybridTausTexture(GLsizei width, GLsizei height);
    }

}
