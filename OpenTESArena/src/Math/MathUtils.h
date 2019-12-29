#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <algorithm>

#include "../Math/Vector3.h"

namespace MathUtils
{
	// A variant of atan2() with a range of [0, 2pi] instead of [-pi, pi].
	double fullAtan2(double y, double x);

	// Converts vertical field of view to camera zoom (where 90 degrees = 1.0 zoom).
	double verticalFovToZoom(double fovY);

	// Converts vertical field of view to horizontal field of view.
	double verticalFovToHorizontalFov(double fovY, double aspectRatio);

	// Finds the intersection of the ray defined by rayStart and rayDirection and the plane
	// defined by pointInPlane and planeNormal if such an intersection exists.
	bool rayPlaneIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &pointInPlane, const Double3 &planeNormal, Double3 &intersection);

	// Finds the intersection of the ray and the quad defined by the vertices.
	bool rayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double3 &v3,
		Double3 &intersection);

	// Finds the shortest distance between points p0 and p1 and between points q0 and q1.
	// - s is the percent distance along p0p1 that's nearest to q0q1
	// - t is the percent distance along q0q1 that's nearest to p0p1
	// - Returns the distance between these two points.
	double distanceBetweenLineSegments(const Double3 &p0, const Double3 &p1,
		const Double3 &q0, const Double3 &q1, double &s, double &t);
}

#endif
