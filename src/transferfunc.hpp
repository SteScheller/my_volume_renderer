#ifndef __TRANSFERFUNC_HPP
#define __TRANSFERFUNC_HPP

#include <set>
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

        virtual bool compare(ControlPointRGBA a, ControlPointRGBA b);
    };

    class ControlPointRGBA1D : public ControlPointRGBA
    {
        public:
        ControlPointRGBA1D();           //!< default constructor
        ControlPointRGBA1D(glm::vec4, float pos);  //!< construction from vector
        ControlPointRGBA1D(             //!< constrution from individual values
                float r, float g, float b, float a, float pos);

        ~ControlPointRGBA1D();          //!< destructor

        static bool compare(ControlPointRGBA1D a, ControlPointRGBA1D b);

        float pos;
        float fderiv;
    };

    typedef std::set<
            ControlPointRGBA1D,
            bool (*) (ControlPointRGBA1D, ControlPointRGBA1D)>
                controlPointSet1D;

    class TransferFuncRGBA1D
    {
        private:
        controlPointSet1D controlPoints;

        public:
        TransferFuncRGBA1D();                    //!< default constructor
        ~TransferFuncRGBA1D();                   //!< destructor

        /*
         * \brief interpolates the assigned RGBA values of a and b based on t
         *
         * Interpolates using cubic hermite polynomials.
         *
         * \param a first control point with a.pos <= b.pos
         * \param b second control point with a.pos <= b.pos
         * \param t position to interpolate at
         *
         * \return interpolated function value
        */
        glm::vec4 interpolateCHermite(
                ControlPointRGBA1D const &a,
                ControlPointRGBA1D const &b,
                float t);

        /*
         * \brief evaluates the transfer function at t
         *
         * \param t position to calculate the function value at
         * \return the function value at t
         *
         * Values of t that are lower than min or higher than max are clamped
         * to the function value at the according limit.
         */
        glm::vec4 operator()(float t);  //!< () operator

        /*
         * \brief inserts a new control point at the given position
         *
         * \param color RGBA color value of the new control point
         * \param t position of the new control point
         */
        void insertControlPoint(glm::vec4 color, float pos);

        /*
         * \brief removes the control point at the given position
         */
        //void removeControlPoint(float pos);

        /*
         * \brief returns a pointer to the set of control points
         */
        controlPointSet1D* getControlPoints();
    };
}

#endif
