#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

namespace MathUtils
{
	// Returns whether the given value is within epsilon of zero.
	double almostZero(double value);

	// Returns whether the two values are within epsilon of each other.
	double almostEqual(double a, double b);

	// A variant of atan2() with a range of [0, 2pi] instead of [-pi, pi].
	double fullAtan2(double y, double x);

	// Converts vertical field of view to camera zoom (where 90 degrees = 1.0 zoom).
	double verticalFovToZoom(double fovY);

	// Converts vertical field of view to horizontal field of view.
	double verticalFovToHorizontalFov(double fovY, double aspectRatio);

	// Returns whether the given point lies in the half space divided at the given divider point.
	bool isPointInHalfSpace(const Double2 &point, const Double2 &dividerPoint, const Double2 &normal);

	// Returns whether the given triangle and circle intersect each other. Assumes triangle points
	// are ordered counter-clockwise.
	bool triangleCircleIntersection(const Double2 &triangleP0, const Double2 &triangleP1,
		const Double2 &triangleP2, const Double2 &circlePoint, double circleRadius);

	// Finds the intersection of a ray on the given plane. Returns success.
	bool rayPlaneIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &planeOrigin, const Double3 &planeNormal, Double3 *outPoint);

	// Finds the intersection of a ray and a quad defined by three vertices. The vertex order
	// must go around the quad (i.e. v0 = top left, v1 = bottom left, v2 = bottom right).
	bool rayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &v0, const Double3 &v1, const Double3 &v2, Double3 *outPoint);

	// Finds the shortest distance between points p0 and p1 and between points q0 and q1.
	// - s is the percent distance along p0p1 that's nearest to q0q1
	// - t is the percent distance along q0q1 that's nearest to p0p1
	// - Returns the distance between these two points.
	double distanceBetweenLineSegments(const Double3 &p0, const Double3 &p1,
		const Double3 &q0, const Double3 &q1, double &s, double &t);
}

#endif
