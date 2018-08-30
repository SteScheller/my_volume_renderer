#include "transferfunc.hpp"

#include <iostream>
#include <glm/glm.hpp>

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::ControlPointRGBA::ControlPointRGBA(){this->color = glm::vec4(0.f);}
tf::ControlPointRGBA::ControlPointRGBA(glm::vec4 color){this->color = color;}
tf::ControlPointRGBA::ControlPointRGBA(float r, float g, float b, float a)
{
    this->color = glm::vec4(r, g, b, a);
}
tf::ControlPointRGBA::~ControlPointRGBA(){}

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::ControlPointRGBA1D::ControlPointRGBA1D() : tf::ControlPointRGBA()
{
    this->pos = 0.f;
}

tf::ControlPointRGBA1D::ControlPointRGBA1D(glm::vec4 color, float pos) :
    tf::ControlPointRGBA(color)
{
    this->pos = pos;
}
tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float r, float g, float b, float a, float pos) :
    tf::ControlPointRGBA(r, g, b, a)
{
    this->pos = pos;
}
tf::ControlPointRGBA1D::~ControlPointRGBA1D(){}

//-----------------------------------------------------------------------------
//  Definitions for TransferFuncRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::TransferFuncRGBA1D::TransferFuncRGBA1D(){this->min = 0.f; this->max = 1.f;}
tf::TransferFuncRGBA1D::TransferFuncRGBA1D(float min, float max)
{
    this->min = min;
    this->max = max;
}
tf::TransferFuncRGBA1D::~TransferFuncRGBA1D(){}

