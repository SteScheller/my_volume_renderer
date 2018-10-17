#include "transferfunc.hpp"

#include <iostream>
#include <utility>
#include <iterator>
#include <cmath>
#include <cassert>

#include <GL/gl3w.h>

#define GLM_FORCE_SWIZZLE
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
tf::ControlPointRGBA::ControlPointRGBA(const tf::ControlPointRGBA &other)
{
    this->color = other.color;
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

bool tf::ControlPointRGBA::operator==(const ControlPointRGBA &other)
{
    return (this->color == other.color);
}

bool tf::ControlPointRGBA::operator!=(const ControlPointRGBA &other)
{
    return !(*this == other);
}

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::ControlPointRGBA1D::ControlPointRGBA1D() :
    tf::ControlPointRGBA()
{
    this->pos = 0.f;
    this->fderiv = 0.f;
}
tf::ControlPointRGBA1D::ControlPointRGBA1D(float pos) :
    tf::ControlPointRGBA()
{
    this->pos = pos;
    this->fderiv = 0.f;
}
tf::ControlPointRGBA1D::ControlPointRGBA1D(float pos, glm::vec4 color) :
    tf::ControlPointRGBA(color)
{
    this->pos = pos;
    this->fderiv = 0.f;
}
tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float pos, float r, float g, float b, float a) :
    tf::ControlPointRGBA(r, g, b, a)
{
    this->pos = pos;
    this->fderiv = 0.f;
}
tf::ControlPointRGBA1D::ControlPointRGBA1D(
        const tf::ControlPointRGBA1D &other) :
    tf::ControlPointRGBA(other)
{
    this->pos = other.pos;
    this->fderiv = other.fderiv;
}
tf::ControlPointRGBA1D::~ControlPointRGBA1D(){}

// definition of inherited virtual functions
bool tf::ControlPointRGBA1D::compare(
        ControlPointRGBA1D a, ControlPointRGBA1D b)
{
    return a.pos < b.pos;
}

bool tf::ControlPointRGBA1D::operator==(const ControlPointRGBA1D &other)
{
    return (    (this->color == other.color)    &&
                (this->pos == other.pos)        &&
                (this->fderiv == other.fderiv));
}

bool tf::ControlPointRGBA1D::operator!=(const ControlPointRGBA1D &other)
{
    return !(*this == other);
}

//-----------------------------------------------------------------------------
//  Definitions for TransferFuncRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
tf::TransferFuncRGBA1D::TransferFuncRGBA1D()
{
    bool (*fn_pt) (ControlPointRGBA1D, ControlPointRGBA1D) =
        tf::ControlPointRGBA1D::compare;

    this->controlPoints = tf::controlPointSet1D(fn_pt);
    this->transferTex = 0;
    this->controlPoints.emplace(0.f, glm::vec4(0.f));
}
tf::TransferFuncRGBA1D::~TransferFuncRGBA1D()
{
    if (this->transferTex != 0)
        glDeleteTextures(1, &(this->transferTex));
}

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
    else if (t >= controlPoints.rbegin()->pos)
        return controlPoints.rbegin()->color;

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
        float pos, glm::vec4 color)
{
    return controlPoints.emplace(pos, color);
}

std::pair<tf::controlPointSet1D::iterator, bool>
    tf::TransferFuncRGBA1D::insertControlPoint(
        ControlPointRGBA1D cp)
{
    return controlPoints.insert(cp);
}

void tf::TransferFuncRGBA1D::removeControlPoint(float pos)
{
    controlPoints.erase(tf::ControlPointRGBA1D(pos));
}

void tf::TransferFuncRGBA1D::removeControlPoint(
        tf::controlPointSet1D::iterator i)
{
    controlPoints.erase(*i);
}

std::pair<tf::controlPointSet1D::iterator, bool>
    tf::TransferFuncRGBA1D::updateControlPoint(
        tf::controlPointSet1D::iterator i, tf::ControlPointRGBA1D cp)
{
    std::pair<tf::controlPointSet1D::iterator, bool> ret;
    tf::ControlPointRGBA1D backup = *i;

    controlPoints.erase(*i);
    ret = controlPoints.insert(cp);

    if (ret.second == true)
        return ret;
    else
        return controlPoints.insert(backup);
}

void tf::TransferFuncRGBA1D::updateTexture(
        float min, float max, unsigned int res)
{
    float step = 0.f;
    float x = 0.f;
    glm::vec4 *fx = nullptr;

    if (res < 2)
        res = 2;

    fx = new glm::vec4[res];

    step = (max - min) / static_cast<float>(res - 1);

    x = min;
    for (size_t i = 0; i < res; ++i)
    {
        x += step;
        fx[i] = (*this)(x);
    }

    if (transferTex != 0)
        glDeleteTextures(1, &transferTex);

    glGenTextures(1, &transferTex);
    glBindTexture(GL_TEXTURE_2D, transferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res, 1, 0, GL_RGBA, GL_FLOAT, fx);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] fx;
}

GLuint tf::TransferFuncRGBA1D::getTexture()
{
    if (transferTex != 0)
        return transferTex;

    // the texture does not exist yet. Create it with a fixed resolution.

    this->updateTexture(
        controlPoints.begin()->pos,
        controlPoints.rbegin()->pos);

    return transferTex;
}
