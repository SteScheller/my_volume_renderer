#pragma once

#include <GL/gl3w.h>

#include "util.hpp"

namespace util
{
    namespace geometry
    {
        class Shape
        {
            public:
            Shape();
            Shape(const Shape& other) = delete;
            Shape& operator=(const Shape& other) = delete;
            Shape(Shape&& other) = delete;
            Shape& operator=(Shape&& other) = delete;
            ~Shape();

            virtual void draw() const;

            protected:
            GLuint m_vertexArrayObject;

            void bind() const;
            void unbind() const;
        };

        class CubeFrame : Shape
        {
            public:
            CubeFrame();
            CubeFrame(const CubeFrame& other) = delete;
            CubeFrame& operator=(const CubeFrame& other) = delete;
            CubeFrame(CubeFrame&& other) = delete;
            CubeFrame& operator=(CubeFrame&& other) = delete;
            ~CubeFrame();

            void draw() const;
        };

        class Cube : Shape
        {
            public:
            Cube();
            Cube(const Cube& other) = delete;
            Cube& operator=(const Cube& other) = delete;
            Cube(Cube&& other) = delete;
            Cube& operator=(Cube&& other) = delete;
            ~Cube();

            void draw() const;
        };

        class Quad : Shape
        {
            public:
            Quad();
            Quad(const Quad& other) = delete;
            Quad& operator=(const Quad& other) = delete;
            Quad(Quad&& other) = delete;
            Quad& operator=(Quad&& other) = delete;
            ~Quad();

            void draw() const;
        };
    }
}
