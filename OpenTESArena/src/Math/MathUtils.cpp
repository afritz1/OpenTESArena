#include <algorithm>
#include <cmath>

#include "Constants.h"
#include "MathUtils.h"

#include "components/debug/Debug.h"

double MathUtils::almostZero(double value)
{
	return std::abs(value) < Constants::Epsilon;
}

double MathUtils::almostEqual(double a, double b)
{
	return MathUtils::almostZero(a - b);
}

double MathUtils::getRealIndex(int bufferSize, double percent)
{
	const double bufferSizeReal = static_cast<double>(bufferSize);

	// Keep the real index in the same array bounds (i.e. if bufferSize is 5, the max is 4.999...).
	const double maxRealIndex = std::max(0.0, bufferSizeReal - Constants::Epsilon);
	return std::clamp(bufferSizeReal * percent, 0.0, maxRealIndex);
}

int MathUtils::getWrappedIndex(int bufferSize, int index)
{
	while (index >= bufferSize)
	{
		index -= bufferSize;
	}

	while (index < 0)
	{
		index += bufferSize;
	}

	return index;
}

Radians MathUtils::fullAtan2(double y, double x)
{
	const Radians angle = std::atan2(y, x);
	return (angle >= 0.0) ? angle : (Constants::TwoPi + angle);
}

Radians MathUtils::fullAtan2(const NewDouble2 &v)
{
	// Flip +X south/+Y west to +X east/+Y north.
	return MathUtils::fullAtan2(-v.x, -v.y);
}

double MathUtils::verticalFovToZoom(Degrees fovY)
{
	return 1.0 / std::tan((fovY * 0.5) * Constants::DegToRad);
}

Degrees MathUtils::verticalFovToHorizontalFov(Degrees fovY, double aspectRatio)
{
	DebugAssert(fovY > 0.0);
	DebugAssert(fovY < 180.0);
	DebugAssert(aspectRatio > 0.0);

	const double halfDim = aspectRatio * std::tan((fovY * 0.50) * Constants::DegToRad);
	return (2.0 * std::atan(halfDim)) * Constants::RadToDeg;
}

bool MathUtils::isPointInHalfSpace(const Double2 &point, const Double2 &dividerPoint,
	const Double2 &normal)
{
	return (point - dividerPoint).normalized().dot(normal) >= 0.0;
}

bool MathUtils::triangleCircleIntersection(const Double2 &triangleP0, const Double2 &triangleP1,
	const Double2 &triangleP2, const Double2 &circlePoint, double circleRadius)
{
	const double circleRadiusSqr = circleRadius * circleRadius;
	const Double2 p0p1 = triangleP1 - triangleP0;
	const Double2 p1p2 = triangleP2 - triangleP1;
	const Double2 p2p0 = triangleP0 - triangleP2;

	// Check if the circle center is inside the triangle.
	const bool circleCenterInTriangle = [&triangleP0, &triangleP1, &triangleP2, &circlePoint,
		&p0p1, &p1p2, &p2p0]()
	{
		const Double2 p0p1Inner = p0p1.leftPerp().normalized();
		const Double2 p1p2Inner = p1p2.leftPerp().normalized();
		const Double2 p2p0Inner = p2p0.leftPerp().normalized();

		return MathUtils::isPointInHalfSpace(circlePoint, triangleP0, p0p1Inner) &&
			MathUtils::isPointInHalfSpace(circlePoint, triangleP1, p1p2Inner) &&
			MathUtils::isPointInHalfSpace(circlePoint, triangleP2, p2p0Inner);
	}();

	if (circleCenterInTriangle)
	{
		return true;
	}

	// Check if any of the triangle vertices are in the circle.
	const bool anyTriangleVertexInCircle = [&triangleP0, &triangleP1, &triangleP2,
		&circlePoint, circleRadiusSqr]()
	{
		auto isVertexInCircle = [&circlePoint, circleRadiusSqr](const Double2 &vertex)
		{
			return (vertex - circlePoint).lengthSquared() <= circleRadiusSqr;
		};

		return isVertexInCircle(triangleP0) || isVertexInCircle(triangleP1) ||
			isVertexInCircle(triangleP2);
	}();

	if (anyTriangleVertexInCircle)
	{
		return true;
	}

	// Check if the circle intersects any of the triangle edges.
	const bool anyEdgeCircleIntersection = [&triangleP0, &triangleP1, &triangleP2,
		&p0p1, &p1p2, &p2p0, &circlePoint, circleRadiusSqr]()
	{
		auto isEdgeIntersectingCircle = [&circlePoint, circleRadiusSqr](
			const Double2 &vStart, const Double2 &vEnd, const Double2 &vDiff)
		{
			// Vector projection, heavily simplified. Project circle point onto edge.
			const Double2 vals = (circlePoint - vStart) * vDiff;
			const double t = (vals.x + vals.y) / vDiff.lengthSquared();
			if ((t >= 0.0) && (t <= 1.0))
			{
				// Projection is inside the line segment. Check distance from circle center.
				const Double2 edgePoint = vStart + (vDiff * t);
				return (edgePoint - circlePoint).lengthSquared() <= circleRadiusSqr;
			}

			// Projection is outside the line segment.
			return false;
		};

		return isEdgeIntersectingCircle(triangleP0, triangleP1, p0p1) ||
			isEdgeIntersectingCircle(triangleP1, triangleP2, p1p2) ||
			isEdgeIntersectingCircle(triangleP2, triangleP0, p2p0);
	}();

	if (anyEdgeCircleIntersection)
	{
		return true;
	}

	return false;
}

bool MathUtils::rayPlaneIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &planeOrigin, const Double3 &planeNormal, Double3 *outPoint)
{
	DebugAssert(rayDirection.isNormalized());
	DebugAssert(planeNormal.isNormalized());

	const double denominator = rayDirection.dot(planeNormal);
	if (!MathUtils::almostZero(denominator))
	{
		const Double3 projection = planeOrigin - rayStart;
		const double t = projection.dot(planeNormal) / denominator;
		if (t >= 0.0)
		{
			// An intersection exists. Find it.
			*outPoint = rayStart + (rayDirection * t);
			return true;
		}
	}

	return false;
}

bool MathUtils::rayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &v0, const Double3 &v1, const Double3 &v2, Double3 *outPoint)
{
	const Double3 v3 = v0 + (v2 - v1);

	// Calculate the normal of the plane which contains the quad.
	const Double3 normal = (v2 - v0).cross(v1 - v0).normalized();

	// Get the intersection of the ray and the plane that contains the quad.
	Double3 hitPoint;
	if (MathUtils::rayPlaneIntersection(rayStart, rayDirection, v0, normal, &hitPoint))
	{
		// The plane intersection is a point co-planar with the quad. Check if the point is
		// within the bounds of the quad.
		const Double3 a = (v1 - v0).cross(hitPoint - v0);
		const Double3 b = (v2 - v1).cross(hitPoint - v1);
		const Double3 c = (v3 - v2).cross(hitPoint - v2);
		const Double3 d = (v0 - v3).cross(hitPoint - v3);

		const double ab = a.dot(b);
		const double bc = b.dot(c);
		const double cd = c.dot(d);

		if (((ab * bc) >= 0.0) && ((bc * cd) >= 0.0))
		{
			*outPoint = hitPoint;
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

std::vector<Int2> MathUtils::bresenhamLine(const Int2 &p1, const Int2 &p2)
{
	const int dx = std::abs(p2.x - p1.x);
	const int dy = std::abs(p2.y - p1.y);
	const int dirX = (p1.x < p2.x) ? 1 : -1;
	const int dirY = (p1.y < p2.y) ? 1 : -1;

	int pointX = p1.x;
	int pointY = p1.y;
	int error = ((dx > dy) ? dx : -dy) / 2;
	const int endX = p2.x;
	const int endY = p2.y;
	std::vector<Int2> points;

	while (true)
	{
		points.push_back(Int2(pointX, pointY));

		if ((pointX == endX) && (pointY == endY))
		{
			break;
		}

		const int innerError = error;

		if (innerError > -dx)
		{
			error -= dy;
			pointX += dirX;
		}

		if (innerError < dy)
		{
			error += dx;
			pointY += dirY;
		}
	}

	return points;
}
