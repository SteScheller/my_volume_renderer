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
                ~Shape();

                virtual void draw() const;

            private:
                GLuint m_vertexArrayObject;

                void bind() const;
                void unbind() const;
        }

        class CubeFrame : Shape
        {
            public:
                CubeFrame();
                ~CubeFrame();

                void draw() const;
        }

        class Cube : Shape
        {
            public:
                Cube();
                ~Cube();

                void draw() const;
        }

        class Quad : Shape
        {
            public:
                Quad();
                ~Quad();

                void draw() const;
        }
    }
}
