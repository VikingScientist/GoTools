/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 *
 * Contact information: E-mail: tor.dokken@sintef.no                      
 * SINTEF ICT, Department of Applied Mathematics,                         
 * P.O. Box 124 Blindern,                                                 
 * 0314 Oslo, Norway.                                                     
 *
 * This file is part of GoTools.
 *
 * GoTools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version. 
 *
 * GoTools is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with GoTools. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In accordance with Section 7(b) of the GNU Affero General Public
 * License, a covered work must retain the producer line in every data
 * file that is created or manipulated using GoTools.
 *
 * Other Usage
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the GoTools library without
 * disclosing the source code of your own applications.
 *
 * This file may be used in accordance with the terms contained in a
 * written agreement between you and SINTEF ICT. 
 */

#include "GoTools/geometry/Ellipse.h"
#include "GoTools/geometry/SplineCurve.h"
#include "GoTools/geometry/GeometryTools.h"
#include "GoTools/geometry/Circle.h"
#include <vector>


using std::vector;
using std::cout;
using std::endl;
using std::streamsize;
using std::swap;


namespace Go
{


//===========================================================================
Ellipse::Ellipse(Point centre, Point direction, Point normal,
                 double r1, double r2,
                 bool isReversed)
    : centre_(centre), vec1_(direction), normal_(normal), r1_(r1), r2_(r2),
      startparam_(0.0), endparam_(2.0*M_PI)
//===========================================================================
{
    if (centre_.dimension() != 3) {
        THROW("Dimension must be 3.");
        return;
    }

    if (dimension() == 3)
        normal_.normalize();
    setSpanningVectors();

    if (isReversed)
        reverseParameterDirection();
}


//===========================================================================
Ellipse::~Ellipse()
//===========================================================================
{
}


//===========================================================================
void Ellipse::read(std::istream& is)
//===========================================================================
{
    bool is_good = is.good();
    if (!is_good) {
        THROW("Invalid geometry file!");
    }

    int dim;
    is >> dim;
    centre_.resize(dim);
    normal_.resize(dim);
    vec1_.resize(dim);
    is >> r1_
       >> r2_
       >> centre_
       >> normal_
       >> vec1_;

    if(dim == 3)
        normal_.normalize();
    setSpanningVectors();

    is >> startparam_ >> endparam_;

    // Need to take care of rounding errors: If pars are "roughly"
    // (0, 2*M_PI) it is probably meant *exactly* (0, 2*M_PI).
    const double pareps = 1.0e-4; // This is admittedly arbitrary...
    if (fabs(startparam_) < pareps) 
      startparam_ = 0.0;
    if (fabs(endparam_ - 2.0*M_PI) < pareps)        
      endparam_ = 2.0 * M_PI;

    // "Reset" reversion
    isReversed_ = false;

    // Swapped flag
    int isReversed; // 0 or 1
    is >> isReversed;
    if (isReversed == 0) {
        // Do nothing
    }
    else if (isReversed == 1) {
        reverseParameterDirection();
    }
    else {
        THROW("Swapped flag must be 0 or 1");
    }
}


//===========================================================================
void Ellipse::write(std::ostream& os) const
//===========================================================================
{
    streamsize prev = os.precision(15);
    int dim = dimension();
    os << dim << endl
       << r1_ << endl
       << r2_ << endl
       << centre_ << endl
       << normal_ << endl
       << vec1_ << endl
       << startparam_ << " " << endparam_ << endl;

    if (!isReversed()) {
        os << "0" << endl;
    }
    else {
        os << "1" << endl;
    }

    os.precision(prev);   // Reset precision to it's previous value
}


//===========================================================================
BoundingBox Ellipse::boundingBox() const
//===========================================================================
{
    // A rather inefficient hack...
    Ellipse* ellipse = const_cast<Ellipse*>(this);
    SplineCurve* tmp = ellipse->geometryCurve();
    BoundingBox box = tmp->boundingBox();
    delete tmp;
    return box;
}

//===========================================================================
int Ellipse::dimension() const
//===========================================================================
{
    return centre_.dimension();
}
    

//===========================================================================
ClassType Ellipse::instanceType() const
//===========================================================================
{
    return classType();
}


//===========================================================================
ClassType Ellipse::classType()
//===========================================================================
{
    return Class_Ellipse;
}


//===========================================================================
Ellipse* Ellipse::clone() const
//===========================================================================
{
    Ellipse* ellipse = new Ellipse(centre_, vec1_, normal_, r1_, r2_,
        isReversed_);
    ellipse->setParamBounds(startparam_, endparam_);
    return ellipse;
}


//===========================================================================
void Ellipse::point(Point& pt, double tpar) const
//===========================================================================
{
    getReversedParameter(tpar);
    pt = centre_ + r1_*cos(tpar)*vec1_ + r2_*sin(tpar)*vec2_;
}


//===========================================================================
void Ellipse::point(std::vector<Point>& pts, 
                    double tpar,
                    int derivs,
                    bool from_right) const
//===========================================================================
{
    DEBUG_ERROR_IF(derivs < 0, 
                   "Negative number of derivatives makes no sense.");
    int totpts = (derivs + 1);
    int ptsz = (int)pts.size();
    DEBUG_ERROR_IF(ptsz < totpts, 
                   "The vector of points must have sufficient size.");

    int dim = dimension();
    for (int i = 0; i < totpts; ++i) {
        if (pts[i].dimension() != dim) {
            pts[i].resize(dim);
        }
        pts[i].setValue(0.0);
    }

    point(pts[0], tpar);
    if (derivs == 0)
        return;

    // Since the ellipse is parametrized as:
    // c(t) = centre_ + r1_*cos(t)*dir1_ + r2_*sin(t)*dir2_,
    // the derivatives follow easily.
    getReversedParameter(tpar);
    double sin_t = sin(tpar);
    double cos_t = cos(tpar);
    for (int ki = 1; ki < derivs + 1; ++ki) {
        double sgn1 = (ki%4 == 1 || ki%4 == 2) ? -1.0 : 1.0;
        double sgn2 = (ki%4 == 2 || ki%4 == 3) ? -1.0 : 1.0;
        pts[ki] = (ki%2 == 1) ? sgn1*r1_*sin_t*vec1_ + sgn2*r2_*cos_t*vec2_ :
            sgn1*r1_*cos_t*vec1_ + sgn2*r2_*sin_t*vec2_;
        // Take reversion into account
        if (isReversed()) {
            double sgnrev = (ki % 2 == 1) ? -1.0 : 1.0;
            pts[ki] *= sgnrev;
        }
    }
}


//===========================================================================
double Ellipse::startparam() const
//===========================================================================
{
    return startparam_;
}


//===========================================================================
double Ellipse::endparam() const
//===========================================================================
{
    return endparam_;
}


//===========================================================================
void Ellipse::swapParameters2D()
//===========================================================================
{
    if (dimension() == 2) {
        swap(centre_[0], centre_[1]);
        swap(vec1_[0], vec1_[1]);
        swap(vec2_[0], vec2_[1]);
    }
}


//===========================================================================
void Ellipse::setParameterInterval(double t1, double t2)
//===========================================================================
{
    MESSAGE("setParameterInterval() doesn't make sense.");
}


//===========================================================================
SplineCurve* Ellipse::geometryCurve()
//===========================================================================
{
    return createSplineCurve();
}


//===========================================================================
SplineCurve* Ellipse::createSplineCurve() const
//===========================================================================
{
    // Based on SISL function s1522.

    double tworoot = sqrt ((double) 2.0);
    double weight  = (double) 1.0 / tworoot;
    double factor = 2.0 * M_PI;

    // Knot vector
    double et[12];
    et[0] = 0.0;
    int i;
    for ( i=1;  i < 3;  i++ ) {
        et[i]     = 0.0;
        et[2 + i] = factor * 0.25;
        et[4 + i] = factor * 0.5;
        et[6 + i] = factor * 0.75;
        et[8 + i] = factor;
    }
    et[11] = factor;

    // Vertices
    double coef[36];
    int dim = dimension();
    Point axis1 = r1_ * vec1_;
    Point axis2 = r2_ * vec2_;
    if (dim == 2) {
        for ( i=0;  i < 2;  i++ ) {
            coef[     i] = centre_[i] + axis1[i];
            coef[3 +  i] = weight*(centre_[i] + axis1[i] + axis2[i]);
            coef[6 +  i] = centre_[i] + axis2[i];
            coef[9 + i] = weight*(centre_[i] - axis1[i] + axis2[i]);
            coef[12 + i] = centre_[i] - axis1[i];
            coef[15 + i] = weight*(centre_[i] - axis1[i] - axis2[i]);
            coef[18 + i] = centre_[i] - axis2[i];
            coef[21 + i] = weight*(centre_[i] + axis1[i] - axis2[i]);
            coef[24 + i] = centre_[i] + axis1[i];
        }
        // The rational weights.
        coef[2] = 1.0;
        coef[5] = weight;
        coef[8] = 1.0;
        coef[11] = weight;
        coef[14] = 1.0;
        coef[17] = weight;
        coef[20] = 1.0;
        coef[23] = weight;
        coef[26] = 1.0;
    }
    else {
        for ( i=0;  i < 3;  i++ ) {
            coef[     i] = centre_[i] + axis1[i];
            coef[4 +  i] = weight*(centre_[i] + axis1[i] + axis2[i]);
            coef[8 +  i] = centre_[i] + axis2[i];
            coef[12 + i] = weight*(centre_[i] - axis1[i] + axis2[i]);
            coef[16 + i] = centre_[i] - axis1[i];
            coef[20 + i] = weight*(centre_[i] - axis1[i] - axis2[i]);
            coef[24 + i] = centre_[i] - axis2[i];
            coef[28 + i] = weight*(centre_[i] + axis1[i] - axis2[i]);
            coef[32 + i] = centre_[i] + axis1[i];
        }
        // The rational weights.
        coef[3] = 1.0;
        coef[7] = weight;
        coef[11] = 1.0;
        coef[15] = weight;
        coef[19] = 1.0;
        coef[23] = weight;
        coef[27] = 1.0;
        coef[31] = weight;
        coef[35] = 1.0;
    }

    int ncoefs = 9;
    int order = 3;
    bool rational = true;
    SplineCurve curve(ncoefs, order, et, coef, dim, rational);

    // Extract segment. We need all this because 'curve' is a SplineCurve
    // with different parametrization than the original Ellipse.
    Point pt1, pt2, clo_pt1, clo_pt2;
    point(pt1, startparam_);
    point(pt2, endparam_);
    if (isReversed())
        swap(pt1, pt2);
    double clo_t1, clo_t2, clo_dist1, clo_dist2;
    double tmin = 0.0;
    double tmax = factor;
    curve.closestPoint(pt1, tmin, tmax,
                       clo_t1, clo_pt1, clo_dist1, &startparam_);
    curve.closestPoint(pt2, tmin, tmax,
                       clo_t2, clo_pt2, clo_dist2, &endparam_);

    SplineCurve* segment = curve.subCurve(clo_t1, clo_t2);
    segment->basis().rescale(startparam_, endparam_);

    if (isReversed())
        segment->reverseParameterDirection();

    return segment;
}


//===========================================================================
bool Ellipse::isDegenerate(double degenerate_epsilon)
//===========================================================================
{
    // We consider an Ellipse as degenerate if either radii is smaller
    // than the epsilon.

    return ((r1_ < degenerate_epsilon) ||
            (r2_ < degenerate_epsilon));
}


//===========================================================================
Ellipse* Ellipse::subCurve(double from_par, double to_par,
                          double fuzzy) const
//===========================================================================
{
    Ellipse* ellipse = clone();
    ellipse->setParamBounds(from_par, to_par);
    return ellipse;
}


//===========================================================================
DirectionCone Ellipse::directionCone() const
//===========================================================================
{
    double tmin = startparam();
    double tmax = endparam();
    vector<Point> pts;
    point(pts, 0.5*(tmin+tmax), 1);
    // We must calculate the angle between the mid point and the end
    // points. As the curvature is monotone this gives the boundaries
    // for the tangents.
    Point start_pt, end_pt;
    point(start_pt, startparam_);
    point(end_pt, endparam_);
    Point dir1 = start_pt - centre_;
    Point dir2 = end_pt - centre_;
    Point dir3 = pts[0] - centre_;
    double ang1 = dir1.angle(dir3);
    double ang2 = dir2.angle(dir3);
    return DirectionCone(pts[1], std::max(fabs(ang1), fabs(ang2)));
}
 

//===========================================================================
void Ellipse::appendCurve(ParamCurve* cv, bool reparam)
//===========================================================================
{
    MESSAGE("appendCurve() not implemented!");
}


//===========================================================================
void Ellipse::appendCurve(ParamCurve* cv,
                          int continuity, double& dist, bool reparam)
//===========================================================================
{
    MESSAGE("appendCurve() not implemented!");
}


//===========================================================================
void Ellipse::closestPoint(const Point& pt,
                           double tmin,
                           double tmax,
                           double& clo_t,
                           Point& clo_pt,
                           double& clo_dist,
                           double const *seed) const
//===========================================================================
{
    // This is a temporary solution...
    // Algorithm:
    // 1) Use circle with centre at the ellipse centre to find guess_param 
    // 2) Use ParamCurve::closestPointGeneric() to find the closest point.

    double radius = centre_.dist(pt);
    Circle* c = new Circle(radius, centre_, normal_, vec1_, isReversed_);
    c->setParamBounds(startparam_, endparam_);
    double guess_param;
    c->closestPoint(pt, tmin, tmax, guess_param, clo_pt, clo_dist);
    ParamCurve::closestPointGeneric(pt, tmin, tmax,
                                    guess_param, clo_t, clo_pt, clo_dist);
    delete c;
}


//===========================================================================
double Ellipse::length(double tol)
//===========================================================================
{
    int num_spans = 4;

    double result = 0.0;
    double tstep = (endparam_ - startparam_)/(double)num_spans;
    for (int ki = 0; ki < num_spans; ++ki)
    {
        double from = startparam_ + ki*tstep;
        double to = from + tstep;
        result += ParamCurve::length(tol, from, to);
    }

    return result;
}


//===========================================================================
void Ellipse::setParamBounds(double startpar, double endpar)
//===========================================================================
{
    double fuzzy = 1.0e-12;
    if (fabs(startpar) < fuzzy)
        startpar = 0.0;
    else if (fabs(2.0*M_PI-startpar) < fuzzy)
        startpar = 2.0*M_PI;
    if (fabs(endpar) < fuzzy)
        endpar = 0.0;
    else if (fabs(2.0*M_PI-endpar) < fuzzy)
        endpar = 2.0*M_PI;

    if (startpar >= endpar)
        THROW("First parameter must be strictly less than second.");
    if (startpar < -2.0 * M_PI || endpar > 2.0 * M_PI)
        THROW("Parameters must be in [-2pi, 2pi].");
    if (endpar - startpar > 2.0 * M_PI)
        THROW("(endpar - startpar) must not exceed 2pi.");

    startparam_ = startpar;
    endparam_ = endpar;
}


//===========================================================================
bool Ellipse::isClosed() const
//===========================================================================
{
  return (endparam_ - startparam_ == 2.0*M_PI);
}


//===========================================================================
void Ellipse::translateCurve(const Point& dir)
//===========================================================================
{
  centre_ += dir;
}

//===========================================================================
void Ellipse::setSpanningVectors()
//===========================================================================
{
    // In 3D, the spanning vectors vec1_, vec2_, and the vector
    // normal_ defines a right-handed coordinate system. Similar to an
    // axis2_placement_3d entity in STEP.

    int dim = centre_.dimension();
    if (dim == 2) {
        vec2_.resize(2);
        vec2_[0] = -vec1_[1];
        vec2_[1] = vec1_[0];
    }
    else if (dim ==3) {
        Point tmp = vec1_ - (vec1_ * normal_) * normal_;
        if (tmp.length() == 0.0) 
            THROW("X-axis parallel to normal.");
        vec1_ = tmp;
        vec2_ = normal_.cross(vec1_);
    }
    else {
        THROW("Dimension must be 2 or 3");
    }
    vec1_.normalize();
    vec2_.normalize();
}

//===========================================================================
bool Ellipse::isInPlane(const Point& norm,
		       double eps, Point& pos) const

//===========================================================================
{
  double ang = norm.angle(normal_);
  pos = centre_;

  return (ang <= eps || fabs(M_PI-ang) <= eps);
}

}
