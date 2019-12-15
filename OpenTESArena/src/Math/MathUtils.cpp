#include <cmath>

#include "Constants.h"
#include "MathUtils.h"

#include "components/debug/Debug.h"

double MathUtils::fullAtan2(double y, double x)
{
	const double angle = std::atan2(y, x);
	return (angle >= 0.0) ? angle : (Constants::TwoPi + angle);
}

double MathUtils::verticalFovToZoom(double fovY)
{
	return 1.0 / std::tan((fovY * 0.5) * Constants::DegToRad);
}

double MathUtils::verticalFovToHorizontalFov(double fovY, double aspectRatio)
{
	DebugAssert(fovY > 0.0);
	DebugAssert(fovY < 180.0);
	DebugAssert(aspectRatio > 0.0);

	const double halfDim = aspectRatio * std::tan((fovY * 0.50) * Constants::DegToRad);
	return (2.0 * std::atan(halfDim)) * Constants::RadToDeg;
}

bool MathUtils::RayPlaneIntersection(const Double3& rayStart, const Double3& rayDirection, const Double3& pointInPlane, const Double3& planeNormal, Double3& intersection)
{
	const Double3 norm = planeNormal.normalized();
	double denominator = rayDirection.normalized().dot(norm);
	if (abs(denominator) > Constants::Epsilon)
	{
		Double3 projection = pointInPlane - rayStart;
		double t = projection.dot(norm) / denominator;
		if (t >= 0)
		{
			// An intersection exists. Find it.
			intersection = rayStart + (rayDirection * t);
			return true;
		}
	}
	return false;
}

bool MathUtils::RayQuadIntersection(const Double3& rayStart, const Double3& rayDirection, const Double3* vertices, Double3 &intersection)
{
	// Calculate the normal of the plane which contains the quad
	Double3 normal = (vertices[2] - vertices[0]).cross(vertices[1] - vertices[0]).normalized();

	// Get the intersection of the ray and the plane that contains the quad
	Double3 planeIntersection;
	if (MathUtils::RayPlaneIntersection(rayStart, rayDirection, vertices[0], normal, planeIntersection))
	{
		// plane intersection is a point coplanar with tthe vertices of the quad (at least with the first 3, which form the basis for the plane the quad is in)
		// Check if the coplanar point is within the bounds of the quad

		Double3 a = (vertices[1] - vertices[0]).cross(planeIntersection - vertices[0]);
		Double3 b = (vertices[2] - vertices[1]).cross(planeIntersection - vertices[1]);
		Double3 c = (vertices[3] - vertices[2]).cross(planeIntersection - vertices[2]);
		Double3 d = (vertices[0] - vertices[3]).cross(planeIntersection - vertices[3]);

		double ab = a.dot(b);
		double bc = b.dot(c);
		double cd = c.dot(d);

		if ((ab * bc >= 0) && (bc * cd >= 0))
		{
			intersection = planeIntersection;
			return true;
		}
	}
	return false;
}

double MathUtils::DistanceBetweenLineSegments(const Double3& p0, const Double3 p1, const Double3& q0, const Double3& q1, double& s, double& t)
{
	auto u = p1 - p0;
	auto v = q1 - q0;

	// These values are needed for the calculation of values s and t
	auto p0q0 = p0 - q0;
	auto _a = u.dot(u);
	auto _b = u.dot(v);
	auto _c = v.dot(v);
	auto _d = u.dot(p0q0);
	auto _e = v.dot(p0q0);

	auto be = _b * _e;
	auto cd = _c * _d;
	auto ac = _a * _c;
	auto ae = _a * _e;
	auto bd = _b * _d;
	auto bb = _b * _b;

	// Calculate s and t. These are the points along u and v from p0 and q0 respectively that are the closest to each other. 
	// The values are limited to the interval [0, 1] because outside of that range is along the line that the segment exists
	// on, but outside the bounds of the segment
	s = std::clamp((be - cd) / (ac - bb), 0.0, 1.0);
	t = std::clamp((ae - bd) / (ac - bb), 0.0, 1.0);

	// Calculate Psc and Qtc. These are the points on their respective segments that are closest to each other.
	auto Psc = p0 + (u * s);
	auto Qtc = p0 + (v * t);

	// The distance between these two points is the shortest distance between the line segments
	return (Psc - Qtc).length();
}