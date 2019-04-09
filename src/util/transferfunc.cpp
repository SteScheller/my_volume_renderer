#include "transferfunc.hpp"

#include <iostream>
#include <utility>
#include <iterator>
#include <cmath>
#include <cassert>

#include <GL/gl3w.h>

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>

#include "util.hpp"

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA
//-----------------------------------------------------------------------------
// Constructors / Destructor
util::tf::ControlPointRGBA::ControlPointRGBA()
{
    this->color = glm::vec4(0.f);
}

util::tf::ControlPointRGBA::ControlPointRGBA(glm::vec4 color)
{
    this->color = color;
}

util::tf::ControlPointRGBA::ControlPointRGBA(glm::vec3 color, float alpha)
{
    this->color = glm::vec4(color, alpha);
}

util::tf::ControlPointRGBA::ControlPointRGBA(
        float r, float g, float b, float a)
{
    this->color = glm::vec4(r, g, b, a);
}

util::tf::ControlPointRGBA::ControlPointRGBA(const ControlPointRGBA &other)
{
    this->color = other.color;
}

util::tf::ControlPointRGBA::~ControlPointRGBA(){}

bool util::tf::ControlPointRGBA::compare(
        __attribute__((unused)) ControlPointRGBA a,
        __attribute__((unused)) ControlPointRGBA b)
{
        std::cout << "Warning: call to empty compare function of control "
            "point base class" << std::endl;
        assert(0);
}

bool util::tf::ControlPointRGBA::operator==(const ControlPointRGBA &other)
{
    return (this->color == other.color);
}

bool util::tf::ControlPointRGBA::operator!=(const ControlPointRGBA &other)
{
    return !(*this == other);
}

//-----------------------------------------------------------------------------
//  Definitions for ControlPointRGBA1D
//-----------------------------------------------------------------------------
// Constructors / Destructor
util::tf::ControlPointRGBA1D::ControlPointRGBA1D() :
    ControlPointRGBA()
{
    this->pos = 0.f;
    this->fderiv = 0.f;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(float pos) :
    ControlPointRGBA()
{
    this->pos = pos;
    this->fderiv = 0.f;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(float pos, glm::vec4 color) :
    ControlPointRGBA(color)
{
    this->pos = pos;
    this->fderiv = 0.f;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float pos, float slope, glm::vec4 color) :
    ControlPointRGBA(color)
{
    this->pos = pos;
    this->fderiv = slope;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float pos, glm::vec3 color, float alpha) :
    ControlPointRGBA(color, alpha)
{
    this->pos = pos;
    this->fderiv = 0.f;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float pos, float slope, glm::vec3 color, float alpha) :
    ControlPointRGBA(color, alpha)
{
    this->pos = pos;
    this->fderiv = slope;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float pos, float r, float g, float b, float a) :
    ControlPointRGBA(r, g, b, a)
{
    this->pos = pos;
    this->fderiv = 0.f;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(
        float pos, float slope, float r, float g, float b, float a) :
    ControlPointRGBA(r, g, b, a)
{
    this->pos = pos;
    this->fderiv = slope;
}

util::tf::ControlPointRGBA1D::ControlPointRGBA1D(
        const ControlPointRGBA1D &other) :
    ControlPointRGBA(other)
{
    this->pos = other.pos;
    this->fderiv = other.fderiv;
}

util::tf::ControlPointRGBA1D::~ControlPointRGBA1D(){}

// definition of inherited virtual functions
bool util::tf::ControlPointRGBA1D::compare(
        ControlPointRGBA1D a, ControlPointRGBA1D b)
{
    return a.pos < b.pos;
}

bool util::tf::ControlPointRGBA1D::operator==(const ControlPointRGBA1D &other)
{
    return (    (this->color == other.color)    &&
                (this->pos == other.pos)        &&
                (this->fderiv == other.fderiv));
}

bool util::tf::ControlPointRGBA1D::operator!=(const ControlPointRGBA1D &other)
{
    return !(*this == other);
}

//-----------------------------------------------------------------------------
//  Definitions for TransferFuncRGBA1D
//-----------------------------------------------------------------------------
util::tf::TransferFuncRGBA1D::TransferFuncRGBA1D()
{
    bool (*fn_pt) (ControlPointRGBA1D, ControlPointRGBA1D) =
        tf::ControlPointRGBA1D::compare;

    m_controlPoints = tf::controlPointSet1D_t(fn_pt);
    m_tfTex = util::texture::Texture2D();
    m_controlPoints.emplace(0.f, glm::vec4(0.f));
    m_controlPoints.emplace(255.f, glm::vec4(1.f));
}

util::tf::TransferFuncRGBA1D::TransferFuncRGBA1D(TransferFuncRGBA1D&& other) :
    m_controlPoints(std::move(other.m_controlPoints)),
    m_tfTex(std::move(other.m_tfTex))
{
}

util::tf::TransferFuncRGBA1D& util::tf::TransferFuncRGBA1D::operator=(
        TransferFuncRGBA1D&& other)
{
    m_controlPoints = std::move(other.m_controlPoints);
    m_tfTex = std::move(other.m_tfTex);

    return *this;
}

util::tf::TransferFuncRGBA1D::~TransferFuncRGBA1D()
{
}

// Member functions
glm::vec4 util::tf::TransferFuncRGBA1D::interpolateCHermite(
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

    glm::vec3 rgbderiv = b.color.rgb() - a.color.rgb();

    glm::mat4 geometry = glm::mat4(
            a.color.r,  a.color.g,  a.color.b,  a.color.a,
            rgbderiv.r, rgbderiv.g, rgbderiv.b, a.fderiv,
            rgbderiv.r, rgbderiv.g, rgbderiv.b, b.fderiv,
            b.color.r,  b.color.g,  b.color.b,  b.color.a);

    interpolated = geometry * hermitePolynomials;

    return interpolated;
}

glm::vec4 util::tf::TransferFuncRGBA1D::operator()(float t)
{
    glm::vec4 value = glm::vec4(0.f);
    ControlPointRGBA1D a, b;

    // handle cases when the transfer function has none or only one
    // control point
    if (m_controlPoints.empty())
        return glm::vec4(0.f);
    else if(m_controlPoints.size() == 1)
        return m_controlPoints.begin()->color;

    // handle cases if t is above the highest or below the lowest control point
    // position
    if (t < m_controlPoints.begin()->pos)
        return m_controlPoints.begin()->color;
    else if (t >= m_controlPoints.rbegin()->pos)
        return m_controlPoints.rbegin()->color;

    // Find the correct pair of control points where t lies in between
    // and interpolate the function value
    for (
            auto i = m_controlPoints.begin(),
                j = std::prev(m_controlPoints.end());
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

util::tf::controlPointSet1D_t*
    util::tf::TransferFuncRGBA1D::accessControlPoints()
{
    return &m_controlPoints;
}

std::pair<util::tf::controlPointSet1D_t::iterator, bool>
    util::tf::TransferFuncRGBA1D::insertControlPoint(
        float pos, glm::vec4 color)
{
    util::tf::ControlPointRGBA1D cp(pos);
    util::tf::controlPointSet1D_t::iterator cpIterator =
        m_controlPoints.find(cp);
    if (m_controlPoints.cend() != cpIterator)
        m_controlPoints.erase(cpIterator);

    return m_controlPoints.emplace(pos, color);
}

std::pair<util::tf::controlPointSet1D_t::iterator, bool>
    util::tf::TransferFuncRGBA1D::insertControlPoint(
        float pos, float slope, glm::vec4 color)
{
    util::tf::ControlPointRGBA1D cp(pos);
    util::tf::controlPointSet1D_t::iterator cpIterator =
        m_controlPoints.find(cp);
    if (m_controlPoints.cend() != cpIterator)
        m_controlPoints.erase(cpIterator);

    return m_controlPoints.emplace(pos, slope, color);
}

std::pair<util::tf::controlPointSet1D_t::iterator, bool>
    util::tf::TransferFuncRGBA1D::insertControlPoint(
        float pos, glm::vec3 color, float alpha)
{
    util::tf::ControlPointRGBA1D cp(pos);
    util::tf::controlPointSet1D_t::iterator cpIterator =
        m_controlPoints.find(cp);
    if (m_controlPoints.cend() != cpIterator)
        m_controlPoints.erase(cpIterator);

    return m_controlPoints.emplace(pos, color, alpha);
}

std::pair<util::tf::controlPointSet1D_t::iterator, bool>
    util::tf::TransferFuncRGBA1D::insertControlPoint(
        float pos, float slope, glm::vec3 color, float alpha)
{
    util::tf::ControlPointRGBA1D cp(pos);
    util::tf::controlPointSet1D_t::iterator cpIterator =
        m_controlPoints.find(cp);
    if (m_controlPoints.cend() != cpIterator)
        m_controlPoints.erase(cpIterator);

    return m_controlPoints.emplace(pos, slope, color, alpha);
}

std::pair<util::tf::controlPointSet1D_t::iterator, bool>
    util::tf::TransferFuncRGBA1D::insertControlPoint(
        ControlPointRGBA1D cp)
{
    util::tf::controlPointSet1D_t::iterator cpIterator =
        m_controlPoints.find(cp);
    if (m_controlPoints.cend() != cpIterator)
        m_controlPoints.erase(cpIterator);

    return m_controlPoints.insert(cp);
}

void util::tf::TransferFuncRGBA1D::removeControlPoint(float pos)
{
    m_controlPoints.erase(tf::ControlPointRGBA1D(pos));
}

void util::tf::TransferFuncRGBA1D::removeControlPoint(
        tf::controlPointSet1D_t::iterator i)
{
    m_controlPoints.erase(*i);
}

std::pair<util::tf::controlPointSet1D_t::iterator, bool>
    util::tf::TransferFuncRGBA1D::updateControlPoint(
        controlPointSet1D_t::iterator i, ControlPointRGBA1D cp)
{
    std::pair<controlPointSet1D_t::iterator, bool> ret;
    ControlPointRGBA1D backup = *i;

    m_controlPoints.erase(*i);
    ret = m_controlPoints.insert(cp);

    if (ret.second == true)
        return ret;
    else
        return m_controlPoints.insert(backup);
}

void util::tf::TransferFuncRGBA1D::updateTexture(
        float min, float max, size_t res)
{
    float step = 0.f;
    float x = 0.f;

    if (res < 2)
        res = 2;

    std::vector<glm::vec4> fx(res, {0.f, 0.f, 0.f, 0.f});

    step = (max - min) / static_cast<float>(res - 1);

    x = min;
    for (size_t i = 0; i < res; ++i)
    {
        x += step;
        fx[i] = (*this)(x);
    }

    m_tfTex = util::texture::Texture2D(
        GL_RGBA,
        GL_RGBA,
        0,
        GL_FLOAT,
        GL_LINEAR,
        GL_CLAMP_TO_EDGE,
        res,
        1,
        fx.data());

}

void util::tf::TransferFuncRGBA1D::updateTexture(size_t res)
{
    updateTexture(
            m_controlPoints.begin()->pos,
            m_controlPoints.rbegin()->pos,
            res);
}

util::texture::Texture2D& util::tf::TransferFuncRGBA1D::accessTexture()
{
    if (m_tfTex.getID() != 0)
        return m_tfTex;

    // the texture does not exist yet. Create it with a fixed resolution.

    this->updateTexture(
        m_controlPoints.begin()->pos,
        m_controlPoints.rbegin()->pos);

    return m_tfTex;
}

util::tf::discreteTf1D_t util::tf::TransferFuncRGBA1D::getDiscretized(
        float min, float max, size_t res)
{
    float step = 0.f;
    float x = 0.f;
    glm::vec4 fx(0.f);

    if (res < 2) res = 2;

    discreteTf1D_t discreteTf(res, { 0.f, 0.f, 0.f, 0.f });

    step = (max - min) / static_cast<float>(res - 1);

    x = min;
    for (size_t i = 0; i < res; ++i)
    {
        x += step;
        fx = (*this)(x);
        discreteTf[i][0] = fx.r;
        discreteTf[i][1] = fx.g;
        discreteTf[i][2] = fx.b;
        discreteTf[i][3] = fx.a;
    }

    return discreteTf;
}
