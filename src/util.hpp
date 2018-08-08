#ifndef __UTIL_HPP
#define __UTIL_HPP

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#define printOpenGLError() util::printOglError(__FILE__, __LINE__)

namespace util
{

    //-------------------------------------------------------------------------
    // Declarations
    //-------------------------------------------------------------------------
    bool printOglError(const char *file, int line);

    //-------------------------------------------------------------------------
    // Templates
    //-------------------------------------------------------------------------
    /*! \brief Transforms cartesian into polar coordinates.
     *  \return A vector where with components (r, phi, theta)
     *
     *  Transforms caresian into a polar coordinate system where phi and theta
     *  are the angles between the x axis and the radius vector.
     */
    template <class T1, typename T2 = float>
    T1 cartesianToPolar(T1& coords)
    {
        T2 r, phi, theta;
        T1 normalized, normalized_xz;
        T2 pi = glm::pi<T2>();
        T2 half_pi = glm::half_pi<T2>();

        T2 zero = static_cast<T2>(0);
        T2 one = static_cast<T2>(1);

        r = glm::length(coords);
        normalized = glm::normalize(coords);
        normalized_xz = glm::normalize(
            T1(coords.x, static_cast<T2>(0), coords.z));
        if (coords.x >= zero)
        {
            phi = glm::acos(
                glm::dot(normalized_xz, T1(one, zero, zero)));
            if (coords.z < zero)
                phi = 2.f * pi - phi;
        }
        else
        {
            phi = glm::acos(
                glm::dot(normalized_xz, T1(-one, zero, zero)));
            if (coords.z > zero)
                phi = pi - phi;
            else
                phi += pi;
        }
        theta = half_pi - glm::acos(
            glm::dot(normalized, T1(zero, one, zero)));

        return T1(r, phi, theta);
    }

    /*! \brief Transforms polar into cartesian coordinates.
     *  \param coords A vector with compononets (r, phi, theta)
     *  \return A vector where with components (x, y, z)
     *
     *  Transforms polar coordinates from a system where phi and theta
     *  are the angles between the x axis and the radius vector into
     *  cartesian coordinates.
     */
    template <class T1>
    T1 polarToCartesian(T1& coords)
    {
        return T1(
            coords.x * glm::cos(coords.z) * glm::cos(coords.y),
            coords.x * glm::sin(coords.z),
            coords.x * glm::cos(coords.z) * glm::sin(coords.y));
    }
}

#endif
