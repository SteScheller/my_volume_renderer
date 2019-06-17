#pragma once

#include <tuple>
#include <vector>
#include <cstddef>
#include <cmath>

#include <boost/multi_array.hpp>

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

    using bin_t = std::tuple<double, double, unsigned int>;
    //-------------------------------------------------------------------------
    // Templated functions
    //-------------------------------------------------------------------------
    /**
     * @brief linearly interpolates between a and b with (t from [0,1])
     * @param a
     * @param b
     * @param t
     */
    template<typename T1, typename T2 = float>
    T2 linearInterpolation(T1 a, T1 b, T2 t)
    {
        return static_cast<T2>(
                static_cast<T2>(a) * (static_cast<T2>(1.0) - t) +
                static_cast<T2>(b) * t);
    }
    /**
     * @brief bilinearly interpolates between a, b, c, d with (x,y from [0,1])
     * @param a
     * @param b
     * @param t
     */
    template<typename T1, typename T2 = float>
    T2 bilinearInterpolation(T1 a, T1 b, T1 c, T1 d, T2 x, T2 y)
    {
        // (0,0) a-----------b (1,0)
        //       |           |
        //       |   o (x,y) |
        //       |           |
        //       |           |
        // (0,1) c-----------d (1,1)

        T2 result = 0.f;

        result = static_cast<T2>(a) * (static_cast<T2>(1.0) - x) *
                    (static_cast<T2>(1.0) - y) +
                 static_cast<T2>(b) * x * (static_cast<T2>(1.0) - y) +
                 static_cast<T2>(c) * (static_cast<T2>(1.0) - x) * y +
                 static_cast<T2>(d) * x * y;

        return result;
    }
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
        double bin_size =
            (static_cast<double>(max - min) + 1.0) /
            static_cast<double>(num_bins);

        // initialize the bins
        for (size_t i = 0; i < num_bins; i++)
        {
            std::get<0>((bins)[i]) =
                static_cast<double>(i) * bin_size -
                0.5 * bin_size +
                static_cast<double>(min);
            std::get<1>((bins)[i]) =
                static_cast<double>(i + 1) * bin_size -
                0.5 * bin_size +
                static_cast<double>(min);
            std::get<2>((bins)[i]) = 0;
        }

        // walk through values and count them in bins
        size_t idx = 0;
        T val;
        for (size_t i = 0; i < num_values; i++)
        {
            val = values[i];

            // place in corresponding bin
            if ((min <= val) && (val <= max))
            {
                idx = static_cast<size_t>(
                    std::round(static_cast<double>(val - min) / bin_size));
                std::get<2>((bins)[idx])++;
            }
        }

        return bins;
    }

    /**
     * /brief creates isoline geometry for a given 2D field
     *
     * /param domain    field data on rectangular 2D domain
     * /param isovalue  threshold for which to extract the isolines
     *
     * /return A vector of lines that form the respective isolines
     *
     * Note: The position of the lines is derivated from the shape of the
     *       field and results in position from
     *       [0, width - 1] x [0, height - 1].
     */
    template<typename T>
    std::vector<util::geometry::Line2D> extractIsolines(
        boost::multi_array<T, 2> domain, T isovalue)
    {
        std::vector<util::geometry::Line2D> lines;
        std::array<float, 4> vertices = {0.f, 0.f, 0.f, 0.f};
        T ul, ur, ll, lr;   // node values (upper-left, upper-right, ...)
        float p1, p2, p3, p4;   // local coordinates of asymptotes
        T m;                // value at the middle of a cell
        unsigned int node_sig = 0;

        for (size_t j = 0; j < (domain.shape()[1] - 1); j++)
        {
            for (size_t i = 0; i < (domain.shape()[0] - 1); i++)
            {
                // compare nodes of the square to the iso value
                ul = domain[j][i];
                ur = domain[j][i + 1];
                ll = domain[j + 1][i];
                lr = domain[j + 1][i + 1];

                node_sig = 0;
                if (ul >= isovalue) node_sig |= 1u;
                if (ur >= isovalue) node_sig |= (1u << 1);
                if (ll >= isovalue) node_sig |= (1u << 2);
                if (lr >= isovalue) node_sig |= (1u << 3);

                if ((node_sig == 0b1111u) || (node_sig == 0b0000u))
                {
                    // all nodes above or below isovalue -> no isoline
                    continue;
                }
                else if ((node_sig == 0b1110u) || (node_sig == 0b0001u))
                {
                    // upper left corner

                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ul) / (ur - ul));
                    p2 = static_cast<float>((isovalue - ul) / (ll - ul));

                    vertices[0] = static_cast<float>(i) + p1; // x1
                    vertices[1] = static_cast<float>(j); // y1
                    vertices[2] = static_cast<float>(i); // x2
                    vertices[3] = static_cast<float>(j) + p2; // y2
                    lines.emplace_back(true, vertices);
                }
                else if ((node_sig == 0b1101u) || (node_sig == 0b0010u))
                {
                    // upper right corner

                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ul) / (ur - ul));
                    p2 = static_cast<float>((isovalue - ur) / (lr - ur));

                    vertices[0] = static_cast<float>(i) + p1; // x1
                    vertices[1] = static_cast<float>(j); // y1
                    vertices[2] = static_cast<float>(i) + 1.f; // x2
                    vertices[3] = static_cast<float>(j) + p2; // y2
                    lines.emplace_back(true, vertices);
                }
                else if ((node_sig == 0b1011u) || (node_sig == 0b0100u))
                {
                    // lower left corner

                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ll) / (lr - ll));
                    p2 = static_cast<float>((isovalue - ul) / (ll - ul));

                    vertices[0] = static_cast<float>(i); // x1
                    vertices[1] = static_cast<float>(j) + p2; // y1
                    vertices[2] = static_cast<float>(i) + p1; // x2
                    vertices[3] = static_cast<float>(j) + 1.f; // y2
                    lines.emplace_back(true, vertices);
                }
                else if ((node_sig == 0b0111u) || (node_sig == 0b1000u))
                {
                    // lower right corner

                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ll) / (lr - ll));
                    p2 = static_cast<float>((isovalue - ur) / (lr - ur));

                    vertices[0] = static_cast<float>(i) + 1.f; // x1
                    vertices[1] = static_cast<float>(j) + p2; // y1
                    vertices[2] = static_cast<float>(i) + p1; // x2
                    vertices[3] = static_cast<float>(j) + 1.f; // y2
                    lines.emplace_back(true, vertices);
                }
                else if ((node_sig == 0b0011u) || (node_sig == 0b1100u))
                {
                    // horizontal
                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ul) / (ll - ul));
                    p2 = static_cast<float>((isovalue - ur) / (lr - ur));

                    vertices[0] = static_cast<float>(i); // x1
                    vertices[1] = static_cast<float>(j) + p1; // y1
                    vertices[2] = static_cast<float>(i) + 1.f; // x2
                    vertices[3] = static_cast<float>(j) + p2; // y2
                    lines.emplace_back(true, vertices);
                }
                else if ((node_sig == 0b1010u) || (node_sig == 0b0101u))
                {
                    // vertical

                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ul) / (ur - ul));
                    p2 = static_cast<float>((isovalue - ll) / (lr - ll));

                    vertices[0] = static_cast<float>(i) + p1; // x1
                    vertices[1] = static_cast<float>(j); // y1
                    vertices[2] = static_cast<float>(i) + p2; // x2
                    vertices[3] = static_cast<float>(j) + 1.f; // y2
                    lines.emplace_back(true, vertices);
                }
                else if ((node_sig == 0b0110u) || (node_sig == 0b1001u))
                {
                    // ambigous diagonal case


                    // calculate local coordinates of asymptotes
                    p1 = static_cast<float>((isovalue - ul) / (ur - ul));
                    p2 = static_cast<float>((isovalue - ul) / (ll - ul));
                    p3 = static_cast<float>((isovalue - ur) / (lr - ur));
                    p4 = static_cast<float>((isovalue - ll) / (lr - ll));

                    m = bilinearInterpolation<T, float>(ul, ur, ll, lr, 0.5f, 0.5f);

                    if (((m >= isovalue) && (node_sig == 0b0110u)) ||
                        ((m < isovalue) && (node_sig == 0b1001u)))
                    {
                        // lines from upper right to lower left
                        vertices[0] = static_cast<float>(i) + p1; // x1
                        vertices[1] = static_cast<float>(j); // y1
                        vertices[2] = static_cast<float>(i); // x2
                        vertices[3] = static_cast<float>(j) + p2; // y2
                        lines.emplace_back(true, vertices);

                        vertices[0] = static_cast<float>(i) + 1.f; // x1
                        vertices[1] = static_cast<float>(j) + p3; // y1
                        vertices[2] = static_cast<float>(i) + p4; // x2
                        vertices[3] = static_cast<float>(j) + 1.f; // y2
                        lines.emplace_back(true, vertices);
                    }
                    else
                    {
                        // lines from upper left to lower right
                        vertices[0] = static_cast<float>(i) + p1; // x1
                        vertices[1] = static_cast<float>(j); // y1
                        vertices[2] = static_cast<float>(i) + 1.f; // x2
                        vertices[3] = static_cast<float>(j) + p3; // y2
                        lines.emplace_back(true, vertices);

                        vertices[0] = static_cast<float>(i); // x1
                        vertices[1] = static_cast<float>(j) + p2; // y1
                        vertices[2] = static_cast<float>(i) + p4; // x2
                        vertices[3] = static_cast<float>(j) + 1.f; // y2
                        lines.emplace_back(true, vertices);
                    }
                }
            }
        }

        return lines;
    }
}

