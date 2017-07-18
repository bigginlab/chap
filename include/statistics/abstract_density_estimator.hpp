#ifndef ABSTRACT_DENSITY_ESTIMATOR_HPP
#define ABSTRACT_DENSITY_ESTIMATOR_HPP

#include "gromacs/utility/real.h"

#include "geometry/spline_curve_1D.hpp"
#include "statistics/kernel_function.hpp"


/*!
 * \brief Helper class for specifying parameters is in the classes derived from
 * AbstractDensityEstimator.
 *
 * This class is used to simplify the interface of the various density 
 * estimation classes. It internally maintains variables for all parameters 
 * that may be used by any of these as well as corresponding flags indicating
 * whether the value of a specific parameter has been set.
 *
 * It is the responsibility of the classes derived from 
 * AbstractDensityEstimator to ensure that a parameter has been properly set
 * before using it. This class does not perform any sanity checks on the 
 * parameter values.
 */
class DensityEstimationParameters
{
    public:

        // constructor and destructor:
        DensityEstimationParameters();

        // setter methods:
        void setBinWidth(real binWidth);
        void setBandWidth(real bandWidth);
        void setMaxEvalPointDist(real maxEvalPointDist);
        void setEvalRangeCutoff(real evalRangeCutoff);
        void setKernelFunction(eKernelFunction kernelFunction);

        // getter methods:
        real binWidth() const;
        bool binWidthIsSet() const; 

        real bandWidth() const;
        bool bandWidthIsSet() const;

        real maxEvalPointDist() const;
        bool maxEvalPointDistIsSet() const;

        real evalRangeCutoff() const;
        bool evalRangeCutoffIsSet() const;

        eKernelFunction kernelFunction() const;
        bool kernelFunctionIsSet() const;


    private:

        real binWidth_;
        bool binWidthIsSet_;

        real bandWidth_;
        bool bandWidthIsSet_;

        real maxEvalPointDist_;
        bool maxEvalPointDistIsSet_;

        real evalRangeCutoff_;
        bool evalRangeCutoffIsSet_;

        eKernelFunction kernelFunction_;
        bool kernelFunctionIsSet_;
    
};


/*!
 * \brief Abstract interface class for density estimation.
 *
 * This class specifies the interface that density estimation classes need to
 * implement. This enables substitution different methods (such as histograms
 * and kernel density estimation for one another without the need to rewrite
 * the code using these classes.
 */
class AbstractDensityEstimator
{
    public:

        // density estimation interface:
        virtual SplineCurve1D estimate(
                std::vector<real> &samples) = 0;

        // setter function for parameters:
        virtual void setParameters(
                const DensityEstimationParameters &params) = 0;

    protected:

        // flags:
        bool parametersSet_ = false;
};


/*!
 * Enum for the various classes derived from AbstractDensityEstimator.
 */
enum eDensityEstimator {eDensityEstimatorHistogram,
                        eDensityEstimatorKernel};

#endif
