#include "transferfunc.hpp"

#include <iostream>
#include <utility>
#include <iterator>
#include <cmath>
#include <cassert>
#include <glm/glm.hpp>

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::ControlPointRGBA::ControlPointRGBA(){color = glm::vec4(0.f);}
tf::ControlPointRGBA::ControlPointRGBA(glm::vec4 color){color = color;}
tf::ControlPointRGBA::ControlPointRGBA(float r, float g, float b, float a)
{
    color = glm::vec4(r, g, b, a);
}
tf::ControlPointRGBA::~ControlPointRGBA(){}

bool tf::ControlPointRGBA::compare(
        __attribute__((unused)) ControlPointRGBA a,
        __attribute__((unused)) ControlPointRGBA b)
{
        std::cout << "Warning: call to empty compare function of control "
            "point base class" << std::endl;
        assert(0);
}

/*bool tf::ControlPointRGBA::operator==(const ControlPointRGBA &other)
{
    return (this->color == other.color);
}

bool tf::ControlPointRGBA::operator!=(const ControlPointRGBA &other)
{
    return !(*this == other);
}*/

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::ControlPointRGBA1D::ControlPointRGBA1D() :
    tf::ControlPointRGBA()
{
    pos = 0.f;
    fderiv = 0.f;
}

tf::ControlPointRGBA1D::ControlPointRGBA1D(glm::vec4 color, float pos) :
    tf::ControlPointRGBA(color)
{
    pos = pos;
    fderiv = 0.f;
}
tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float r, float g, float b, float a, float pos) :
    tf::ControlPointRGBA(r, g, b, a)
{
    pos = pos;
    fderiv = 0.f;
}
tf::ControlPointRGBA1D::~ControlPointRGBA1D(){}

// definition of inherited virtual functions
bool tf::ControlPointRGBA1D::compare(
        ControlPointRGBA1D a, ControlPointRGBA1D b)
{
    return a.pos < b.pos;
}

/*bool tf::ControlPointRGBA1D::operator==(const ControlPointRGBA1D &other)
{
    return (    (this->color == other.color)    &&
                (this->pos == other.pos)        &&
                (this->fderiv == other.fderiv));
}

bool tf::ControlPointRGBA1D::operator!=(const ControlPointRGBA1D &other)
{
    return !(*this == other);
}*/

//-----------------------------------------------------------------------------
//  Definitions for TransferFuncRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::TransferFuncRGBA1D::TransferFuncRGBA1D()
{
    bool (*fn_pt) (ControlPointRGBA1D, ControlPointRGBA1D) =
        tf::ControlPointRGBA1D::compare;

    controlPoints = tf::controlPointSet1D(fn_pt);
}
tf::TransferFuncRGBA1D::~TransferFuncRGBA1D(){}

// Member functions
glm::vec4 tf::TransferFuncRGBA1D::interpolateCHermite(
        ControlPointRGBA1D const &a, ControlPointRGBA1D const &b, float t)
{
    glm::vec4 interpolated = glm::vec4(0.f);
    float t3 = std::pow(t, 3);
    float t2 = std::pow(t, 2);

    glm::vec4 hermitePolynomials = glm::vec4(
            2.f * t3 - 3.f * t2 + 1.f,
            t3 - 2.f * t2 + t,
            t3 - t2,
            -2.f * t3 + 3 * t2);

    glm::mat4 geometry = glm::mat4(
            a.color.r,  a.color.g,  a.color.b,  a.color.a,
            a.fderiv,   a.fderiv,   a.fderiv,   a.fderiv,
            b.fderiv,   b.fderiv,   b.fderiv,   b.fderiv,
            b.color.r,  b.color.g,  b.color.b,  b.color.a);

    interpolated = geometry * hermitePolynomials;

    return interpolated;
}

glm::vec4 tf::TransferFuncRGBA1D::operator()(float t)
{
    glm::vec4 value = glm::vec4(0.f);
    tf::ControlPointRGBA1D a, b;

    // handle cases when the transfer function has none or only one
    // control point
    if (controlPoints.empty())
        return glm::vec4(0.f);
    else if(controlPoints.size() == 1)
        return controlPoints.begin()->color;

    // handle cases if t is above the highest or below the lowest control point
    // position
    if (t < controlPoints.begin()->pos)
        return controlPoints.begin()->color;
    else if (t >= controlPoints.end()->pos)
        return controlPoints.end()->color;

    // Find the correct pair of control points where t lies in between
    // and interpolate the function value
    for (
            auto i = controlPoints.begin(),
                j = std::prev(controlPoints.end());
            i != j;
            ++i)
    {
        a = *i;
        b = *(std::next(i));

        if ((t >= a.pos) && (t < b.pos))
        {
            value = interpolateCHermite(a, b, (t - a.pos) / (b.pos - a.pos));
            break;
        }
    }

    return value;
}

tf::controlPointSet1D* tf::TransferFuncRGBA1D::getControlPoints()
{
    return &controlPoints;
}

std::pair<tf::controlPointSet1D::iterator, bool>
    tf::TransferFuncRGBA1D::insertControlPoint(
        glm::vec4 color, float pos)
{
    return controlPoints.emplace(color, pos);
}

std::pair<tf::controlPointSet1D::iterator, bool>
    tf::TransferFuncRGBA1D::insertControlPoint(
        ControlPointRGBA1D cp)
{
    return controlPoints.insert(cp);
}

void tf::TransferFuncRGBA1D::removeControlPoint(float pos)
{
    controlPoints.erase(tf::ControlPointRGBA1D(glm::vec4(1.f), pos));
}

void tf::TransferFuncRGBA1D::removeControlPoint(
        tf::controlPointSet1D::iterator i)
{
    controlPoints.erase(*i);
}

std::pair<tf::controlPointSet1D::iterator, bool>
    tf::TransferFuncRGBA1D::updateControlPoint(
        tf::controlPointSet1D::iterator i, ControlPointRGBA1D cp)
{

    controlPoints.erase(*i);
    return controlPoints.insert(cp);
}

