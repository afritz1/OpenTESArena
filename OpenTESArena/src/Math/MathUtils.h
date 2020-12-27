#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <type_traits>
#include <vector>

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../World/VoxelUtils.h"

#include "components/utilities/Bytes.h"

using Radians = double;
using Degrees = double;

namespace MathUtils
{
	// Returns whether the given value is within epsilon of zero.
	double almostZero(double value);

	// Returns whether the two values are within epsilon of each other.
	double almostEqual(double a, double b);

	// Returns whether the given value represents a number on the number line, including infinity.
	template <typename T>
	bool isValidFloatingPoint(T value)
	{
		static_assert(std::is_floating_point_v<T>);
		return !std::isnan(value);
	}

	// Returns whether the given integer is a power of 2.
	template <typename T>
	constexpr bool isPowerOf2(T value)
	{
		static_assert(std::is_integral_v<T>);
		return Bytes::getSetBitCount(value) == 1;
	}

	// Gets a real (not integer) index in an array from the given percent.
	double getRealIndex(int bufferSize, double percent);

	// Gets the wrapped index within the buffer's range. I.e., if the buffer size is 5
	// and the index is 5, it will return 0.
	int getWrappedIndex(int bufferSize, int index);

	// A variant of atan2() with a range of [0, 2pi] instead of [-pi, pi].
	Radians fullAtan2(double y, double x);
	Radians fullAtan2(const NewDouble2 &v);

	// Converts vertical field of view to camera zoom (where 90 degrees = 1.0 zoom).
	double verticalFovToZoom(Degrees fovY);

	// Converts vertical field of view to horizontal field of view.
	Degrees verticalFovToHorizontalFov(Degrees fovY, double aspectRatio);

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

	// Generates a list of points along a Bresenham line. Only signed integers can be
	// used in a Bresenham's line due to the error calculation.
	std::vector<Int2> bresenhamLine(const Int2 &p1, const Int2 &p2);
}

#endif
