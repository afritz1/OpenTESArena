#include <cmath>

#include "Constants.h"
#include "MathUtils.h"

#include "components/debug/Debug.h"

double MathUtils::almostEqual(double a, double b)
{
	return std::abs(a - b) < Constants::Epsilon;
}

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

bool MathUtils::rayPlaneIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &pointInPlane, const Double3 &planeNormal, Double3 &intersection)
{
	DebugAssert(rayDirection.isNormalized());
	DebugAssert(planeNormal.isNormalized());
	const double denominator = rayDirection.dot(planeNormal);
	if (std::abs(denominator) > Constants::Epsilon)
	{
		const Double3 projection = pointInPlane - rayStart;
		const double t = projection.dot(planeNormal) / denominator;
		if (t >= 0)
		{
			// An intersection exists. Find it.
			intersection = rayStart + (rayDirection * t);
			return true;
		}
	}

	return false;
}

bool MathUtils::rayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double3 &v3,
	Double3 &intersection)
{
	// Calculate the normal of the plane which contains the quad.
	const Double3 normal = (v2 - v0).cross(v1 - v0).normalized();

	// Get the intersection of the ray and the plane that contains the quad.
	Double3 planeIntersection;
	if (MathUtils::rayPlaneIntersection(rayStart, rayDirection, v0, normal, planeIntersection))
	{
		// Plane intersection is a point co-planar with the vertices of the quad (at least
		// with the first 3, which form the basis for the plane the quad is in). Check if the
		// coplanar point is within the bounds of the quad.
		const Double3 a = (v1 - v0).cross(planeIntersection - v0);
		const Double3 b = (v2 - v1).cross(planeIntersection - v1);
		const Double3 c = (v3 - v2).cross(planeIntersection - v2);
		const Double3 d = (v0 - v3).cross(planeIntersection - v3);

		const double ab = a.dot(b);
		const double bc = b.dot(c);
		const double cd = c.dot(d);

		if (((ab * bc) >= 0.0) && ((bc * cd) >= 0.0))
		{
			intersection = planeIntersection;
			return true;
		}
	}

	return false;
}

double MathUtils::distanceBetweenLineSegments(const Double3 &p0, const Double3 &p1,
	const Double3 &q0, const Double3 &q1, double &s, double &t)
{
	const Double3 u = p1 - p0;
	const Double3 v = q1 - q0;

	// These values are needed for the calculation of values s and t.
	const Double3 p0q0 = p0 - q0;
	const double a = u.dot(u);
	const double b = u.dot(v);
	const double c = v.dot(v);
	const double d = u.dot(p0q0);
	const double e = v.dot(p0q0);

	const double be = b * e;
	const double cd = c * d;
	const double ac = a * c;
	const double ae = a * e;
	const double bd = b * d;
	const double bb = b * b;

	// Calculate s and t. These are the points along u and v from p0 and q0 respectively that
	// are the closest to each other. The values are limited to the interval [0, 1] because
	// outside of that range is along the line that the segment exists on, but outside the bounds
	// of the segment.
	s = std::clamp((be - cd) / (ac - bb), 0.0, 1.0);
	t = std::clamp((ae - bd) / (ac - bb), 0.0, 1.0);

	// Calculate Psc and Qtc. These are the points on their respective segments that are closest
	// to each other.
	const Double3 Psc = p0 + (u * s);
	const Double3 Qtc = p0 + (v * t);

	// The distance between these two points is the shortest distance between the line segments.
	return (Psc - Qtc).length();
}
