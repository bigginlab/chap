#ifndef SPLINE_CURVE_1D_HPP
#define SPLINE_CURVE_1D_HPP

#include <vector>

#include <gtest/gtest_prod.h>   

#include <gromacs/math/vec.h>

#include "geometry/abstract_spline_curve.hpp"

#include "geometry/bspline_basis_set.hpp" // TODO to mother class


/*!
 * \brief Spline curve in one dimension.
 *
 * This class represents a spline curve in one spatial dimension, i.e. a spline
 * function. In three dimensions, the class SplineCurve3D can be used.
 */
class SplineCurve1D : public AbstractSplineCurve
{
    friend class SplineCurve1DTest;
    FRIEND_TEST(SplineCurve1DTest, SplineCurve1DFindIntervalTest);

    public:
    
        BSplineBasisSet B_; // TODO: move to mother class

        // constructor and destructor:
        SplineCurve1D(
                int degree, 
                std::vector<real> knotVector,
                std::vector<real> ctrlPoints); 
        SplineCurve1D();

        // public interface for curve evalaution:
        real evaluate(const real &eval, unsigned int deriv);

        // getter function for control points:
        std::vector<real> ctrlPoints() const;


    private:

        // internal variables:
        std::vector<real> ctrlPoints_;

        // auxiliary functions for evaluation:
        real evaluateInternal(const real &eval, unsigned int deriv);
        real evaluateExternal(const real &eval, unsigned int deriv);
        real computeLinearCombination(const SparseBasis &basis);
};

#endif

