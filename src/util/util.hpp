#pragma once

#include <tuple>
#include <vector>
#include <cstddef>

#include "GL/gl3w.h"

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <FreeImage.h>

#include "geometry.hpp"
#include "texture.hpp"
#include "transferfunc.hpp"

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#define printOpenGLError() util::printOglError(__FILE__, __LINE__)

namespace util
{

    //-------------------------------------------------------------------------
    // Declarations
    //-------------------------------------------------------------------------
    class FramebufferObject;
    // texture.cpp
    // see texture classes and functions in texture.hpp

    // geometry.cpp
    // see shape classes and functions in geometry.hpp

    // transferfunc.cpp
    // see transferfunction and control point class in transferfunc.hpp

    // util.cpp
    bool printOglError(const char *file, int line);

    bool checkFile(const std::string& path);

    void makeScreenshot(
        const FramebufferObject &fbo,
        unsigned int width,
        unsigned int height,
        const std::string &file,
        FREE_IMAGE_FORMAT type);

    //-------------------------------------------------------------------------
    // Type definitions
    //-------------------------------------------------------------------------
    class FramebufferObject
    {
        public:
        FramebufferObject();
        FramebufferObject(
            std::vector<util::texture::Texture2D> &&textures,
            const std::vector<GLenum> &attachments);
        FramebufferObject(const FramebufferObject& other) = delete;
        FramebufferObject(FramebufferObject&& other);
        FramebufferObject& operator=(const FramebufferObject& other) = delete;
        FramebufferObject& operator=(FramebufferObject&& other);
        ~FramebufferObject();

        void bind() const;
        void bindRead(size_t attachmentNumber) const;
        void unbind() const;

        const std::vector<GLenum> getAttachments() { return m_attachments; }
        const std::vector<util::texture::Texture2D>& accessTextures()
        {
            return m_textures;
        }

        private:
        GLuint m_ID;
        std::vector<util::texture::Texture2D> m_textures;
        std::vector<GLenum> m_attachments;

    };

    using bin_t = std::tuple<float, float, unsigned int>;
    //-------------------------------------------------------------------------
    // Templated functions
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

    /**
     * /brief create an vector of bins from the given data
     *
     * /param bins   number of bins
     * /param min    minimum value
     * /param max    maximum value
     * /param values pointer to data values
     * /param numValues number of values in the vector pointed to by values.
     *
     * /return An vector of tuples which contain the limits and the count of
     *         the corresponding bin.
     *
     * Creates an vector of bins/ tuples from the given data which can be used
     * to create a histogram. Each bin is a tuple with three components where
     * the first two components contain the limits of the covered interval
     * [first, second) and the third components contains the count.
     *
     * Note: - the interval of the last bin includes the upper limit
     *       [first, second] with second = max
     *       - the calling function has to delete the bins to free the used
     *       memory
     */
    template<class T>
    std::vector<bin_t> binData(
        size_t num_bins,
        T min,
        T max,
        T* values,
        size_t num_values)
    {
        if (num_bins == 0 || min > max || values == nullptr || num_values == 0)
            return std::vector<bin_t>(0);

        std::vector<bin_t> bins(num_bins);
        float bin_size =
            static_cast<float>(max - min) / static_cast<float>(num_bins);

        // initialize the bins
        for (size_t i = 0; i < num_bins; i++)
        {
            std::get<0>((bins)[i]) =
                static_cast<float>(i) * bin_size + static_cast<float>(min);
            std::get<1>((bins)[i]) =
                static_cast<float>(i + 1) * bin_size + static_cast<float>(min);
            std::get<2>((bins)[i]) = 0;
        }

        // walk through values and count them in bins
        size_t idx = 0;
        T val;
        for (size_t i = 0; i < num_values; i++)
        {
            val = values[i];

            // place in corresponding bin
            if ((min <= val) && (val < max))
            {
                idx = static_cast<size_t>(
                    floor(static_cast<float>(val - min) / bin_size));
                std::get<2>((bins)[idx])++;
            }
            else if (val == max)
            {
                // last bin includes upper limit
                std::get<2>((bins)[num_bins - 1])++;
            }
        }

        return bins;
    }
}

