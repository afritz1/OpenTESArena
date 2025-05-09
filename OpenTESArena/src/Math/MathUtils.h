#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <bit>
#include <cmath>
#include <type_traits>
#include <vector>

#include "Constants.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/Bytes.h"

using Radians = double;
using RadiansF = float;
using Degrees = double;
using DegreesF = float;

namespace MathUtils
{
	// Returns whether the given value is within epsilon of zero.
	constexpr double almostZero(double value)
	{
		return value <= Constants::Epsilon && value >= -Constants::Epsilon;
	}

	// Returns whether the two values are within epsilon of each other.
	constexpr double almostEqual(double a, double b)
	{
		return MathUtils::almostZero(a - b);
	}

	// Returns whether the given value represents a number on the number line, including infinity.
	template<typename T>
	bool isValidFloatingPoint(T value)
	{
		static_assert(std::is_floating_point_v<T>);
		return !std::isnan(value);
	}

	template<typename T>
	constexpr bool isPowerOf2(T value)
	{
		if constexpr (std::is_unsigned_v<T>)
		{
			return std::has_single_bit(value);
		}
		else
		{
			if (value < 0)
			{
				value = -value;
			}

			return std::has_single_bit<std::make_unsigned_t<T>>(value);
		}
	}

	// Rounds towards +inf for positive and -inf for negative.
	template<typename T>
	constexpr T roundToGreaterPowerOf2(T value)
	{
		if constexpr (std::is_unsigned_v<T>)
		{
			return std::bit_ceil(value);
		}
		else
		{
			if (value >= 0)
			{
				return std::bit_ceil<std::make_unsigned_t<T>>(value);
			}
			else
			{
				return -static_cast<T>(std::bit_ceil<std::make_unsigned_t<T>>(-value));
			}
		}
	}

	// Rounds towards zero for positive and negative.
	template<typename T>
	constexpr T roundToLesserPowerOf2(T value)
	{
		if constexpr (std::is_unsigned_v<T>)
		{
			return std::bit_floor(value);
		}
		else
		{
			if (value >= 0)
			{
				return std::bit_floor<std::make_unsigned_t<T>>(value);
			}
			else
			{
				return -static_cast<T>(std::bit_floor<std::make_unsigned_t<T>>(-value));
			}
		}
	}

	constexpr Radians degToRad(Degrees degrees)
	{
		return degrees * (Constants::Pi / 180.0);
	}

	constexpr Degrees radToDeg(Radians radians)
	{
		return radians * (180.0 / Constants::Pi);
	}

	Radians safeDegToRad(Degrees degrees);

	// Gets a real (not integer) index in an array from the given percent.
	double getRealIndex(int bufferSize, double percent);

	// Gets the wrapped index within the buffer's range. I.e., if the buffer size is 5
	// and the index is 5, it will return 0.
	int getWrappedIndex(int bufferSize, int index);

	// A variant of atan2() with a range of [0, 2pi] instead of [-pi, pi].
	Radians fullAtan2(double y, double x);
	Radians fullAtan2(const WorldDouble2 &v);

	// Converts vertical field of view to camera zoom (where 90 degrees = 1.0 zoom).
	double verticalFovToZoom(Degrees fovY);

	// Converts vertical field of view to horizontal field of view.
	Degrees verticalFovToHorizontalFov(Degrees fovY, double aspectRatio);

	// Converts yaw (0 - 360) and pitch (-90 - 90) to a 3D coordinate frame.
	void populateCoordinateFrameFromAngles(Degrees yaw, Degrees pitch, Double3 *outForward, Double3 *outRight, Double3 *outUp);

	// Returns whether the given point lies in the half space divided at the given divider point.
	bool isPointInHalfSpace(const Double2 &point, const Double2 &planePoint, const Double2 &planeNormal);
	bool isPointInHalfSpace(const Double3 &point, const Double3 &planePoint, const Double3 &planeNormal);

	// Returns whether the two line segments intersect.
	bool lineSegmentIntersection(const Double2 &a0, const Double2 &a1, const Double2 &b0, const Double2 &b1);

	// Returns whether the given triangle and circle intersect each other. Assumes triangle points
	// are ordered counter-clockwise.
	bool triangleCircleIntersection(const Double2 &triangleP0, const Double2 &triangleP1,
		const Double2 &triangleP2, const Double2 &circlePoint, double circleRadius);

	// Returns whether the given triangle and rectangle intersect each other. Assumes triangle points
	// are ordered counter-clockwise.
	bool triangleRectangleIntersection(const Double2 &triangleP0, const Double2 &triangleP1,
		const Double2 &triangleP2, const Double2 &rectLow, const Double2 &rectHigh);

	// Finds the intersection of a ray on the given plane. Returns success.
	bool rayPlaneIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &planeOrigin, const Double3 &planeNormal, double *outT);

	// Finds the intersection of a ray with the given triangle. Returns success.
	bool rayTriangleIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &v0, const Double3 &v1, const Double3 &v2, double *outT);

	// Finds the intersection of a ray and a quad defined by three vertices. The vertex order
	// must go around the quad (i.e. v0 = top left, v1 = bottom left, v2 = bottom right).
	bool rayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &v0, const Double3 &v1, const Double3 &v2, double *outT);

	// Finds the intersection of a ray and a box.
	bool rayBoxIntersection(const Double3 &rayStart, const Double3 &rayDirection, const Double3 &boxCenter,
		double width, double height, double depth, Radians yRotation, double *outT);

	// Returns the signed distance of the point to the plane (can be negative).
	double distanceToPlane(const Double3 &point, const Double3 &planePoint, const Double3 &planeNormal);

	// Finds the shortest distance between points p0 and p1 and between points q0 and q1.
	// - s is the percent distance along p0p1 that's nearest to q0q1
	// - t is the percent distance along q0q1 that's nearest to p0p1
	// - Returns the distance between these two points.
	double distanceBetweenLineSegments(const Double3 &p0, const Double3 &p1,
		const Double3 &q0, const Double3 &q1, double &s, double &t);

	// Generates a list of points along a Bresenham line. Only signed integers can be
	// used in a Bresenham's line due to the error calculation.
	std::vector<Int2> bresenhamLine(const Int2 &p1, const Int2 &p2);

	// Gets the X and Y coordinates from a Z value in a Z-order curve. Used with quadtree node look-up.
	Int2 getZOrderCurvePoint(int index);
}

namespace MathUtilsF
{
	constexpr RadiansF degToRad(DegreesF degrees)
	{
		return degrees * (ConstantsF::Pi / 180.0f);
	}

	constexpr DegreesF radToDeg(RadiansF radians)
	{
		return radians * (180.0f / ConstantsF::Pi);
	}
}

#endif
