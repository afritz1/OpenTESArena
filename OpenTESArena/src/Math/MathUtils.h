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

	// Finds the intersecion of the ray defined by rayStart and rayDirection and the plane defined by pointInPlane and planeNormal if such an intersection exists
	bool RayPlaneIntersection(const Double3& rayStart, const Double3& rayDirection, const Double3& pointInPlane, const Double3& planeNormal, Double3& intersection);

	// Finds the intersection of the ray and the quad defined by the vertices. <vertices> must contain at least 4 Double3 values (only the first 4 are used), and the first 3 are used to determine the plane in which they reside
	bool RayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection, const Double3* vertices, Double3 &intersection);

	// Finds the shortest distance between the line segment between the points p0 and p1 and thee line segment between the points q0 and q1.
	// out parameters:
	//   - s (the percent distance along the line segment p0p1 that's nearest to the line segment q0q1) 
	//   - t (the percent distance along the line segment q0q1 that's nearest to the line segment p0p1)
	// 
	// Returns the distance between these two points
	double DistanceBetweenLineSegments(const Double3& p0, const Double3 p1, const Double3& q0, const Double3& q1, double &s, double &t);
}

#endif
