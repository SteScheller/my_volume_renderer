#ifndef __TRANSFERFUNC_HPP
#define __TRANSFERFUNC_HPP

#include <vector>
#include <glm/glm.hpp>

namespace tf
{
    class ControlPointRGBA
    {
        public:
        ControlPointRGBA();                 //!< default constructor
        ControlPointRGBA(glm::vec4 color);  //!< construction from vector
        ControlPointRGBA(       //!< constrution from individual values
                float r, float g, float b, float a);

        ~ControlPointRGBA();    //!< destructor

        glm::vec4 color;        //!< RGBA color value assigned to that point
    };

    class ControlPointRGBA1D : public ControlPointRGBA
    {
        public:
        ControlPointRGBA1D();           //!< default constructor
        ControlPointRGBA1D(glm::vec4, float pos);  //!< construction from vector
        ControlPointRGBA1D(             //!< constrution from individual values
                float r, float g, float b, float a, float pos);

        ~ControlPointRGBA1D();          //!< destructor

        float pos;
    };

    class TransferFuncRGBA1D
    {
        private:
        float min;
        float max;

        public:
        TransferFuncRGBA1D();                    //!< default constructor
        TransferFuncRGBA1D(float min, float max);//!< constructor with limits
        ~TransferFuncRGBA1D();                   //!< destructor
    };
}

#endif
