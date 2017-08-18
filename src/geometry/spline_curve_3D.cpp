#include <algorithm>
#include <iostream>
#include <iomanip>
#include <limits>

#include <ctime> // TODO: temove this

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/math/tools/minima.hpp>
#include <boost/math/tools/roots.hpp>

#include "geometry/spline_curve_3D.hpp"
#include "geometry/cubic_spline_interp_3D.hpp"


/*
 * Constructor.
 */
SplineCurve3D::SplineCurve3D(int degree,
                             std::vector<real> knotVector,
                             std::vector<gmx::RVec> ctrlPoints)
{
    nCtrlPoints_ = ctrlPoints.size();
    nKnots_ = knotVector.size();
    degree_ = degree;
    arcLengthTableAvailable_ = false;

    // ensure minimal number of control points for given degree:
    if( nCtrlPoints_ < degree_ + 1 )
    {
        std::cerr<<"ERROR: Need at least d + 1 control points!"<<std::endl;
        std::cerr<<"d = "<<degree_
                 <<" and n = "<<nCtrlPoints_
                 <<std::endl;
        std::abort();
    }

    // ensure minimal number of knots:
    if( nKnots_ < nCtrlPoints_ + degree_ + 1 )
    {
        std::cerr<<"ERROR: Need at least n + d + 1 knots!"<<std::endl;
        std::cerr<<"d = "<<degree_
                 <<" and n = "<<nCtrlPoints_
                 <<" and k = "<<nKnots_
                 <<std::endl;
        std::abort();
    }

    // assign knot vector and control points:
    knotVector_ = knotVector;
    knots_ = knotVector;
    ctrlPoints_ = ctrlPoints;
}


/*
 * Default constructor for initialiser lists.
 */
SplineCurve3D::SplineCurve3D()
{

}


/*
 *
 */
SplineCurve3D::SplineCurve3D(const SplineCurve3D &other)
{
    nCtrlPoints_ = other.nCtrlPoints_;
    nKnots_ = other.nKnots_;
    degree_ = other.degree_;

    ctrlPoints_ = other.ctrlPoints_;

    arcLengthTableAvailable_ = other.arcLengthTableAvailable_;
    arcLengthTable_ = other.arcLengthTable_;
}


/*
 * Destructor.
 */
SplineCurve3D::~SplineCurve3D()
{

}


/*
 * Public interface for spline curve evaluation. This function Will return the 
 * (vector-valued) value of the spline curve (or its derivative) at a given
 * evaluation point.
 */
gmx::RVec
SplineCurve3D::evaluate(real &evalPoint,
                        unsigned int derivOrder,
                        eSplineEvalMethod method)
{
    // initialise return value:
    gmx::RVec value(0.0, 0.0, 0.0);

    // create individual vectors for each dimension:
    // TODO: is it more efficient to do this in constructor?
    std::vector<real> ctrlCoefsX;
    std::vector<real> ctrlCoefsY;
    std::vector<real> ctrlCoefsZ;
    for(int i = 0; i < nCtrlPoints_; i++)
    {
        ctrlCoefsX.push_back(ctrlPoints_.at(i)[0]);
        ctrlCoefsY.push_back(ctrlPoints_.at(i)[1]);
        ctrlCoefsZ.push_back(ctrlPoints_.at(i)[2]);
    } 

    // evaluate spline function in each dimension:
    real valueX = evaluateSplineFun(evalPoint, ctrlCoefsX, derivOrder, method);
    real valueY = evaluateSplineFun(evalPoint, ctrlCoefsY, derivOrder, method);
    real valueZ = evaluateSplineFun(evalPoint, ctrlCoefsZ, derivOrder, method);

    // return result in vectorial form:
    return gmx::RVec(valueX, valueY, valueZ);
}


/*
 * Public evalaution interface conveniently defined as an operator.
 */
gmx::RVec
SplineCurve3D::operator()(real &evalPoint,
                          unsigned int derivOrder,
                          eSplineEvalMethod method)
{
    return evaluate(evalPoint, derivOrder, method);
}



/*
 *
 */
gmx::RVec
SplineCurve3D::evaluate(
        const real &eval,
        unsigned int deriv)
{
    // extrapolation or interpolation?
    if( eval < knots_.front() || eval > knots_.back() )
    {
        return evaluateExternal(eval, deriv);
    }
    else
    {
        return evaluateInternal(eval, deriv);
    }
}


/*
 *
 */
gmx::RVec 
SplineCurve3D::evaluateInternal(const real &eval, unsigned int deriv)
{
    // container for basis functions or derivatives:
    SparseBasis basis;

    // derivative required?
    if( deriv == 0 )
    {
        // evaluate B-spline basis:
        basis = B_(eval, knots_, degree_);
    }
    else
    {
        // evaluate B-spline basis derivatives:
        basis = B_(eval, knots_, degree_, deriv); 
    }
    
    // return value of spline curve (derivative) at given evalaution point:
    return computeLinearCombination(basis);
}


/*
 *
 */
gmx::RVec 
SplineCurve3D::evaluateExternal(const real &eval, unsigned int deriv)
{   
    // which boundary is extrapolation based on?
    real boundary;
    if( eval < knots_.front() )
    {
        boundary = knots_.front();
    }
    else
    {
        boundary = knots_.back();
    }

    // derivative required?
    if( deriv == 0 )
    {
        // compute slope and offset:
        // TODO: this can be made mor efficient by evaluating basis and derivs
        // in one go!
        SparseBasis basis = B_(boundary, knots_, degree_);
        gmx::RVec offset = computeLinearCombination(basis);
        basis = B_(boundary, knots_, degree_, 1);
        gmx::RVec slope = computeLinearCombination(basis);

        // return extrapolation point:
        svmul(eval - boundary, slope, slope);
        rvec_add(slope, offset, slope);
        return slope;

    }
    else if( deriv == 1 )
    {
        // simply return the slope at the endpoint:
        SparseBasis basis = B_(boundary, knots_, degree_, 1);
        return computeLinearCombination(basis);
    }
    else
    {
        // for linear extrapolation, second and higher order deriv are zero:
        return gmx::RVec(0.0, 0.0, 0.0);
    }
}


/*
 *
 */
gmx::RVec
SplineCurve3D::computeLinearCombination(const SparseBasis &basis)
{
    gmx::RVec value(gmx::RVec(0.0, 0.0, 0.0)); 
    for(auto b : basis)
    {
        gmx::RVec tmp;
        svmul(b.second, ctrlPoints_[b.first], tmp);
        rvec_add(value, tmp, value);
    }

    return value;
}



/*
 * Change the internal representation of the curve such that it is 
 * parameterised in terms of arc length.
 */
void
SplineCurve3D::arcLengthParam()
{
    // number of uniformly spaced arc length parameters:
    int nNew = 10*nCtrlPoints_;

    // create lookup table for arc length at knots:
    prepareArcLengthTable();
   
    // determine uniform arc length spacing:
    real arcLenStep = arcLengthTable_.back() / (nNew - 1);

    // initialise new control points and parameters:
    std::vector<real> newParams;
    std::vector<gmx::RVec> newPoints;


    // loop over uniformly spaced arc length intervals:
    for(int i = 0; i < nNew; i++)
    {
        // calculate target arc length:
        real newParam = i*arcLenStep;
        newParams.push_back(newParam);

        // find parameter value corresponding to arc length value:
        real oldParam = arcLengthToParam(newParam); 

        //  evaluate spline to get new point:
        newPoints.push_back(this -> evaluate(oldParam, 0, eSplineEvalDeBoor));
    }

    // interpolate new points to get arc length parameterised curve:
    CubicSplineInterp3D Interp;
    SplineCurve3D newSpl = Interp(newParams, 
                                  newPoints, 
                                  eSplineInterpBoundaryHermite);

    // update own parameters:
    this -> knotVector_ = newSpl.knotVector_;
    this -> ctrlPoints_ = newSpl.ctrlPoints_;
    this -> nKnots_ = newSpl.nKnots_;
    this -> nCtrlPoints_ = newSpl.nCtrlPoints_;
    this -> arcLengthTableAvailable_ = false;
}


/*
 * Calculates the length along the arc of the curve between the two parameter
 * values a and b.
 */
real
SplineCurve3D::length(real &lo, real &hi)
{ 
    // do we need to form a lookup table:
    if( arcLengthTableAvailable_ == false || arcLengthTable_.size() == 0 )
    {
       prepareArcLengthTable(); 
    }

    // initialise length as zero::
    real length = 0.0;   
  
    // find intervals of evaluation points:
    int idxLo = findInterval(lo);
    int idxHi = findInterval(hi);

    // add distance in endpoint intervals:
    if( idxHi == idxLo )
    {
        length += arcLengthBoole(lo, hi);
    }
    else
    {
        length += arcLengthBoole(lo, knotVector_[idxLo + 1]);
        length += arcLengthBoole(knotVector_[idxHi], hi);
    }

    // if necessary, loop over intermediate spline segments and sum up lengths:
    if( idxHi - idxLo > 1 )
    {
        for(int i = idxLo + 1; i < idxHi; i++)
        {
            // add length of current segment from lookup table:
            length += arcLengthTable_[i + 1] - arcLengthTable_[i];
        }
    }

    return length;
}


/*
 * Convenience function to calculate arc length of curve between first and last
 * support point.
 */
real 
SplineCurve3D::length()
{
    return length(knotVector_.front(), knotVector_.back());
}


/*
 * Returns the tangent vector at the given evaluation point. Simply a wrapper
 * around the general evaluation function requesting the first derivative at 
 * the given point.
 */
gmx::RVec
SplineCurve3D::tangentVec(real &evalPoint)
{
    return evaluate(evalPoint, 1, eSplineEvalDeBoor); 
}

/*
 * TODO: implement this!
 */
/*
gmx::RVec
SplineCurve3D::normalVec(real &evalPoint)
{
    return evaluate(evalPoint, 1, eSplineEvalDeBoor); 
}
*/


/*
 * Returns the speed of the curve at the given evaluation point, where speed
 * refers to the magnitude of the tangent vector.
 */
real
SplineCurve3D::speed(real &evalPoint)
{
    // calculate tangent vector:
    gmx::RVec tangentVec = this -> tangentVec(evalPoint);

    // return magnitude of tangent vector:
    return std::sqrt(tangentVec[0]*tangentVec[0] + 
                     tangentVec[1]*tangentVec[1] +
                     tangentVec[2]*tangentVec[2]);
}


/*
 *
 */
int
SplineCurve3D::closestCtrlPoint(gmx::RVec &point)
{
    // index of closest ctrlPoint:
    int idx = -1;   // initialised as negative to provoke errors

    // initialise shortest distance:
    real shortestDist = std::numeric_limits<real>::infinity();

    // loop over all control points:
    for(size_t i = 0; i < ctrlPoints_.size(); i++)
    {
        // calculate distance to this point:
        real sqDist = (ctrlPoints_[i][0] - point[0])*(ctrlPoints_[i][0] - point[0]) +
                      (ctrlPoints_[i][1] - point[1])*(ctrlPoints_[i][1] - point[1]) +
                      (ctrlPoints_[i][2] - point[2])*(ctrlPoints_[i][2] - point[2]);

        
        // smaller distance found?
        if( sqDist < shortestDist )
        {
            idx = i;
            shortestDist = sqDist;
        }
    }

    // return index to closest point:
    return idx;
}


/*
 * Takes point in cartesian coordinates and returns that points coordinates in
 * the curvilinear system defined by the spline curve. Return value is an RVec,
 * which contains the following information:
 *
 *      [0] - distance along the arc of the curve
 *      [1] - distance from the curve at closest point
 *      [2] - angular coordinate; this is not yet implemented
 *
 * Note that this function assumes that the curve is parameterised by arc 
 * length!
 *
 * TODO: implement angular coordinate!
 * TODO: remove tolerance argument, this is superseded by Boosts bit argument
 */
gmx::RVec 
SplineCurve3D::cartesianToCurvilinear(gmx::RVec cartPoint,
                                      real lo,
                                      real hi, 
                                      real /*tol*/)
{
    // internal parameters:
    boost::uintmax_t maxIter = 100;

    // objective functionbinding:
    boost::function<real(real)> objFun;
    objFun = boost::bind(&SplineCurve3D::pointSqDist, 
                         this, 
                         cartPoint, 
                         _1);
 
    // find minimum via Brent's method:
    int bits = std::numeric_limits<real>::digits;
    std::pair<real, real> result;
    result = boost::math::tools::brent_find_minima(objFun,
                                                   lo, 
                                                   hi,
                                                   bits,
                                                   maxIter);

    // return curvilinear coordinates of point:
    // TODO: implement angular coordinate
    gmx::RVec curvPoint;
    curvPoint[0] = result.first;
    curvPoint[1] = result.second;
    curvPoint[2] = std::nan("");
    return curvPoint;
}


/*!
 * Getter function for access to the spline curves control points.
 */
std::vector<gmx::RVec>
SplineCurve3D::ctrlPoints() const
{
    return ctrlPoints_;
}


/*
 * Uses Newton-Cotes quadrature of curve speed to determine the length of the 
 * arc between two given parameter values. The specific quadrature rule applied
 * is Boole's rule.
 */
real
SplineCurve3D::arcLengthBoole(real lo, real hi)
{
    // determine intermediate evaluation points:
    real h = (hi - lo)/4.0;
    real t2 = lo + 1.0*h;
    real t3 = lo + 2.0*h;
    real t4 = lo + 3.0*h;

    // evaluate speed at support points: 
    real s1 = speed(lo);
    real s2 = speed(t2);
    real s3 = speed(t3);
    real s4 = speed(t4);
    real s5 = speed(hi);

    // evaluate Boole's law:
    return 2.0*h/45.0*(7.0*s1 + 32.0*s2 + 12.0*s3 + 32.0*s4 + 7.0*s5);
}


/*
 * Prepares a lookup table that associates an arc length with each knot, where
 * the first knot is assigned an arc length of zero.
 */
void
SplineCurve3D::prepareArcLengthTable()
{
    arcLengthTable_.resize(knotVector_.size());
    real segmentLength;
    for(unsigned int i = 0; i < knotVector_.size() - 1; i++)
    {
        // calculate length of current segment:
        segmentLength = arcLengthBoole(knotVector_[i], knotVector_[i+1]);

        // add to arc length table:
        arcLengthTable_[i + 1] = arcLengthTable_[i] + segmentLength;
    }

    // set flag:
    arcLengthTableAvailable_ = true;
}


/*
 * Returns arc length value at the control points by simply removing the 
 * repeated knot values from the arc length lookup lookup table.
 */
std::vector<real>
SplineCurve3D::ctrlPointArcLength()
{
    // check availability:
    if( arcLengthTableAvailable_ == false )
    {
        prepareArcLengthTable();
    }

    // create copy of table and remove redundant elements:
    std::vector<real> arcLength(arcLengthTable_.begin() + degree_, 
                                arcLengthTable_.end() - degree_);

    return arcLength;
}


/*
 * Returns arc length value at first control point.
 */
real
SplineCurve3D::frstPointArcLength()
{
    // check availability:
    if( arcLengthTableAvailable_ == false )
    {
        prepareArcLengthTable();
    }

    return arcLengthTable_.front();
}


/*
 * Returns arc length value at last control point.
 */
real
SplineCurve3D::lastPointArcLength()
{
    // check availability:
    if( arcLengthTableAvailable_ == false )
    {
        prepareArcLengthTable();
    }

    return arcLengthTable_.back();
}


/*
 * Returns the parameter value (in the current parameterisation, typically 
 * chord length) that corresponds to a given value of arc length. This is done
 * via bisection.
 */
real
SplineCurve3D::arcLengthToParam(real &arcLength)
{
    boost::uintmax_t maxIter = 100;
    real absTol = 0.01*std::sqrt(std::numeric_limits<real>::epsilon());

    // sanity check for arc length table:
    if( arcLengthTableAvailable_ != true )
    {
        prepareArcLengthTable();
    }

    // handle upper endpoint:
    if( arcLength == arcLengthTable_.back() )
    {
        return knotVector_.back();
    }

    // find apropriate interval:
    std::pair<std::vector<real>::iterator, std::vector<real>::iterator> bounds;
    bounds = std::equal_range(arcLengthTable_.begin(), arcLengthTable_.end(), arcLength);
    int idxLo = bounds.second - arcLengthTable_.begin() - 1;
    int idxHi = bounds.second - arcLengthTable_.begin();


    // handle query outside table range:
    // TODO: add case for query below lower bound and test!
    if( bounds.second == arcLengthTable_.end() )
    {
        return knotVector_.back() + arcLength - arcLengthTable_.back();
    }
    if( bounds.second == arcLengthTable_.begin() )
    {
        std::cerr<<"ERROR: arc length below table value range!"<<std::endl;
        std::abort();
    }

    // initialise bisection interval and lower limit:
    real tLo = knotVector_[idxLo];
    real tHi = knotVector_[idxHi];
    real tLi = tLo;
   
    // target arc length within this interval:
    real targetIntervalLength = arcLength - arcLengthTable_[idxLo];

    // termination condition binding:
    boost::function<bool(real, real)> termCond;
    termCond = boost::bind(&SplineCurve3D::arcLengthToParamTerm, 
                           this, 
                           _1, 
                           _2,
                           absTol);

    // objective functionbinding:
    boost::function<real(real)> objFun;
    objFun = boost::bind(&SplineCurve3D::arcLengthToParamObj, 
                         this, 
                         tLi, 
                         _1, 
                         targetIntervalLength);

        
    std::pair<real, real> result;
    result = boost::math::tools::toms748_solve(objFun,
                                               tLo, 
                                               tHi,
                                               termCond,
                                               maxIter);
    

    // best guess if middle of bracketing interval:
    return 0.5*(result.first + result.second);
}


/*
 *
 */
bool
SplineCurve3D::arcLengthToParamTerm(real lo, real hi, real tol)
{
    return std::abs(hi - lo) <= tol;
}


/*
 *
 */
real
SplineCurve3D::arcLengthToParamObj(real lo, real hi, real target)
{
    return arcLengthBoole(lo, hi) - target;
}


/*
 * Computes the squared Euclidean distance between some point and the point 
 * on the spline curve with the given parameter value. For efficiency, the
 * square root is not drawn, hence the squared distance is returned.
 */
real
SplineCurve3D::pointSqDist(gmx::RVec point, real eval)
{
    // evaluate spline:
    gmx::RVec splPoint = evaluate(eval, 0, eSplineEvalDeBoor);

    // return squared distance:
    return (splPoint[0] - point[0])*(splPoint[0] - point[0]) +
           (splPoint[1] - point[1])*(splPoint[1] - point[1]) +
           (splPoint[2] - point[2])*(splPoint[2] - point[2]);
}

