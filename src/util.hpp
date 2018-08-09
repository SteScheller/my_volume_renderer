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

    /**
     * /brief create an array of bins from the given data
     *
     * /param bins   number of bins
     * /param min    minimum value
     * /param max    maximum value
     * /param values pointer to data values
     * /param numValues number of values in the array pointed to by values.
     *
     * /return An array of bins with 3 components each [first, second) #third
     * Creates an array of bins from the given data which can be used to create
     * a histogram.
     */
    template<class T>
    void binData( unsigned int bins, T min, T max,
                       T** values, unsigned int numValues ) {
        if (bins == 0 || min > max || values == nullptr || numValues == 0 ) {
            return;
        }

        // create bins
        /*  Bins are created as an array of pairs. Meaning of pairs is as follows:
        *   first value:    lower bound of the corresponding bin
        *   second value:   number of elements in the bin */
        std::pair<float, unsigned int>* binsHistogramm = new std::pair<float, unsigned int>[bins];
        T val;
        float binSize = static_cast<float>(max - min) / static_cast<float>(bins);
        unsigned int idx = 0;

        for (unsigned int i = 0; i < bins; i++)
        {
            binsHistogramm[i].first = i * binSize;
            binsHistogramm[i].second = 0;
        }

        // walk through values and count them in bins
        maxBinValue = 0;
        for (unsigned int i = 0; i < numValues; i++)
        {
            val = (*values)[i];
            // search for corresponding bin
            for (unsigned int j = 0; j < bins; j++)
            {
                if (    (j < (bins - 1)) &&
                        (binsHistogramm[j].first <= static_cast<float>(val)) &&
                        (static_cast<float>(val) < (binsHistogramm[j].first + binSize))     )
                {
                    binsHistogramm[j].second++;
                    break;
                }
                else if ((binsHistogramm[j].first <= static_cast<float>(val)) &&
                    (static_cast<float>(val) <= (binsHistogramm[j].first + binSize)))
                {
                    binsHistogramm[j].second++;
                    break;
                }
                // save maximum value
                if (binsHistogramm[j].second > maxBinValue)
                    maxBinValue = binsHistogramm[j].second;
            }
        }

        // create vertices from histogram data
        std::vector<float> histoVertices;
        float x = 0.f, y = 0.f;
        for (unsigned int i = 0; i < bins; i++)
        {
            x = (   binsHistogramm[i].first - static_cast<float>(min)
                    + 0.5f * binSize    )
                / static_cast<float>(max - min);
            y = static_cast<float>(binsHistogramm[i].second);

            histoVertices.push_back(x);
            histoVertices.push_back(y);
        }

        vaHisto.Create(bins);
        vaHisto.SetArrayBuffer(0, GL_FLOAT, 2, histoVertices.data(), GL_STATIC_DRAW);

        delete[] binsHistogramm;
    }
};


}

#endif
