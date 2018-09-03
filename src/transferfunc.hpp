#ifndef __TRANSFERFUNC_HPP
#define __TRANSFERFUNC_HPP

#include <set>
#include <utility>
#include <iterator>
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

        /*
         * \brief key comparison function for sorting of control points
         *
         * Key comparison function for use with stl containers for determining
         * the order and uniqueness of elements.
         */
        virtual bool compare(ControlPointRGBA a, ControlPointRGBA b);

        /*
         * \brief equality comparison that compares all attributes
         *
         * \return true if all attributes of the compared control points are
         *         equal.
         */
        //virtual bool operator==(const ControlPointRGBA &other);
        //virtual bool operator!=(const ControlPointRGBA &other);
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
        float fderiv;

        /*
         * \brief key comparison function for sorting of control points
         *
         * Key comparison function for use with stl containers for determining
         * the order and uniqueness of elements.
         */
        static bool compare(ControlPointRGBA1D a, ControlPointRGBA1D b);

        /*
         * \brief equality comparison that compares all attributes
         *
         * \return true if all attributes of the compared control points are
         *         equal.
         */
        //bool operator==(const ControlPointRGBA1D &other);
        //bool operator!=(const ControlPointRGBA1D &other);
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
         * \brief returns a pointer to the set of control points
         */
        controlPointSet1D* getControlPoints();

        /*
         * \brief inserts a new control point at the given position
         *
         * \param color RGBA color value of the new control point
         * \param t position of the new control point
         *
         * \return A pair consisting of an iterator to the inserted element
         * (or to the element that prevented the insertion) and a bool value
         * set to true if the insertion took place.
         */
        std::pair<controlPointSet1D::iterator, bool> insertControlPoint(
                glm::vec4 color, float pos);

        /*
         * \brief adds the given control point to the transfer function
         *
         * Adds the given control point to the set of control points of the
         * transfer function if there does not already exist a control point
         * with at the same position. Differences in the color or alpha value
         * are not considered.
         *
         * \param cp control point object which shall be added
         *
         * \return A pair consisting of an iterator to the inserted element
         * (or to the element that prevented the insertion) and a bool value
         * set to true if the insertion took place.
         */
        std::pair<controlPointSet1D::iterator, bool> insertControlPoint(
                ControlPointRGBA1D cp);

        /*
         * \brief removes the control point at the given position
         *
         * \param pos 1D spatial position of the control point
         */
        void removeControlPoint(float pos);

        /*
         * \brief removes the control point at the iterator position
         *
         * \param i iterator position in control point set
         */
        void removeControlPoint(controlPointSet1D::iterator i);

        /*
         * \brief Updates the color and alpha value of the control point
         *
         * \param i iterator position of the control point that shall be
         *          updated
         * \param cp control point object that contains the changes
         *
         * \return A pair consisting of an iterator to the inserted element
         * (or to the element that prevented the insertion) and a bool value
         * set to true if the insertion took place.
         */
        std::pair<tf::controlPointSet1D::iterator, bool> updateControlPoint(
                controlPointSet1D::iterator i, ControlPointRGBA1D cp);
    };
}

#endif
