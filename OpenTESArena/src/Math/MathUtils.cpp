#include <algorithm>

#include "MathUtils.h"

#include "components/debug/Debug.h"

Radians MathUtils::safeDegToRad(Degrees degrees)
{
	const Radians radians = degToRad(degrees);
	if (!std::isfinite(radians))
	{
		return 0.0;
	}

	return radians;
}

double MathUtils::getRealIndex(int bufferSize, double percent)
{
	DebugAssert(bufferSize > 0);
	const double bufferSizeReal = static_cast<double>(bufferSize);

	// Keep the real index in the same array bounds (i.e. if bufferSize is 5, the max is 4.999...).
	const double maxRealIndex = std::max(0.0, bufferSizeReal - Constants::Epsilon);
	return std::clamp(bufferSizeReal * percent, 0.0, maxRealIndex);
}

int MathUtils::getWrappedIndex(int bufferSize, int index)
{
	DebugAssert(bufferSize > 0);

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

Radians MathUtils::fullAtan2(const WorldDouble2 &v)
{
	// Flip +X south/+Y west to +X east/+Y north.
	return MathUtils::fullAtan2(-v.x, -v.y);
}

double MathUtils::verticalFovToZoom(Degrees fovY)
{
	return 1.0 / std::tan(MathUtils::degToRad(fovY * 0.5));
}

Degrees MathUtils::verticalFovToHorizontalFov(Degrees fovY, double aspectRatio)
{
	DebugAssert(fovY > 0.0);
	DebugAssert(fovY < 180.0);
	DebugAssert(aspectRatio > 0.0);

	const double halfDim = aspectRatio * std::tan(MathUtils::degToRad(fovY * 0.50));
	return MathUtils::radToDeg(2.0 * std::atan(halfDim));
}

void MathUtils::populateCoordinateFrameFromAngles(Degrees yaw, Degrees pitch, Double3 *outForward, Double3 *outRight, Double3 *outUp)
{
	const Radians angleXRadians = MathUtils::degToRad(yaw);
	const Radians angleYRadians = MathUtils::degToRad(pitch);
	const double sinePitch = std::sin(angleYRadians);
	const double cosinePitch = std::cos(angleYRadians);
	const double sineYaw = std::sin(angleXRadians);
	const double cosineYaw = std::cos(angleXRadians);
	*outForward = Double3(cosinePitch * sineYaw, sinePitch, cosinePitch * cosineYaw).normalized();
	*outRight = Double3(-cosineYaw, 0.0, sineYaw).normalized();
	*outUp = outRight->cross(*outForward).normalized();
}

bool MathUtils::isPointInHalfSpace(const Double2 &point, const Double2 &planePoint, const Double2 &planeNormal)
{
	return (point - planePoint).dot(planeNormal) >= 0.0;
}

bool MathUtils::isPointInHalfSpace(const Double3 &point, const Double3 &planePoint, const Double3 &planeNormal)
{
	return (point - planePoint).dot(planeNormal) >= 0.0;
}

bool MathUtils::lineSegmentIntersection(const Double2 &a0, const Double2 &a1, const Double2 &b0, const Double2 &b1)
{
	const Double2 aDiff = a1 - a0;
	const Double2 bDiff = b1 - b0;
	const double dotPerp = (aDiff.x * bDiff.y) - (aDiff.y * bDiff.x);
	if (std::abs(dotPerp) < Constants::Epsilon)
	{
		// Line segments are parallel.
		return false;
	}

	const Double2 abDiff = b0 - a0;
	const double s = ((abDiff.x * aDiff.y) - (abDiff.y * aDiff.x)) / dotPerp;
	if ((s < 0.0) || (s > 1.0))
	{
		// Intersection is outside line segment A.
		return false;
	}

	const double t = ((abDiff.x * bDiff.y) - (abDiff.y * bDiff.x)) / dotPerp;
	if ((t < 0.0) || (t > 1.0))
	{
		// Intersection is outside line segment B.
		return false;
	}

	return true;
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

bool MathUtils::triangleRectangleIntersection(const Double2 &triangleP0, const Double2 &triangleP1,
	const Double2 &triangleP2, const Double2 &rectLow, const Double2 &rectHigh)
{
	const Double2 triangleP0P1 = triangleP1 - triangleP0;
	const Double2 triangleP1P2 = triangleP2 - triangleP1;
	const Double2 triangleP2P0 = triangleP0 - triangleP2;

	const Double2 trianglePerp0 = triangleP0P1.rightPerp();
	const Double2 trianglePerp1 = triangleP1P2.rightPerp();
	const Double2 trianglePerp2 = triangleP2P0.rightPerp();

	const Double2 rectP0 = rectLow;
	const Double2 rectP1(rectP0.x + (rectHigh.x - rectLow.x), rectP0.y);
	const Double2 rectP2 = rectHigh;
	const Double2 rectP3(rectP0.x, rectP0.y + (rectHigh.y - rectLow.y));

	auto isInTriangle = [&triangleP0, &triangleP1, &triangleP2, &trianglePerp0, &trianglePerp1, &trianglePerp2](const Double2 &p)
	{
		return MathUtils::isPointInHalfSpace(p, triangleP0, trianglePerp0) &&
			MathUtils::isPointInHalfSpace(p, triangleP1, trianglePerp1) &&
			MathUtils::isPointInHalfSpace(p, triangleP2, trianglePerp2);
	};

	// Check if rectangle is completely inside triangle.
	if (isInTriangle(rectP0) && isInTriangle(rectP1) && isInTriangle(rectP2) && isInTriangle(rectP3))
	{
		return true;
	}

	auto isInRect = [&rectLow, &rectHigh](const Double2 &p)
	{
		return (p.x >= rectLow.x) && (p.x <= rectHigh.x) && (p.y >= rectLow.y) && (p.y <= rectHigh.y);
	};

	// Check if triangle is completely inside rectangle.
	if (isInRect(triangleP0) && isInRect(triangleP1) && isInRect(triangleP2))
	{
		return true;
	}

	// Check if any triangle line segment intersects any rectangle line segment.
	const bool isTriangleP0P1Intersecting =
		MathUtils::lineSegmentIntersection(triangleP0, triangleP1, rectP0, rectP1) ||
		MathUtils::lineSegmentIntersection(triangleP0, triangleP1, rectP1, rectP2) ||
		MathUtils::lineSegmentIntersection(triangleP0, triangleP1, rectP2, rectP3) ||
		MathUtils::lineSegmentIntersection(triangleP0, triangleP1, rectP3, rectP0);
	const bool isTriangleP1P2Intersecting =
		MathUtils::lineSegmentIntersection(triangleP1, triangleP2, rectP0, rectP1) ||
		MathUtils::lineSegmentIntersection(triangleP1, triangleP2, rectP1, rectP2) ||
		MathUtils::lineSegmentIntersection(triangleP1, triangleP2, rectP2, rectP3) ||
		MathUtils::lineSegmentIntersection(triangleP1, triangleP2, rectP3, rectP0);
	const bool isTriangleP2P0Intersecting =
		MathUtils::lineSegmentIntersection(triangleP2, triangleP0, rectP0, rectP1) ||
		MathUtils::lineSegmentIntersection(triangleP2, triangleP0, rectP1, rectP2) ||
		MathUtils::lineSegmentIntersection(triangleP2, triangleP0, rectP2, rectP3) ||
		MathUtils::lineSegmentIntersection(triangleP2, triangleP0, rectP3, rectP0);

	if (isTriangleP0P1Intersecting || isTriangleP1P2Intersecting || isTriangleP2P0Intersecting)
	{
		return true;
	}

	return false;
}

bool MathUtils::rayPlaneIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &planeOrigin, const Double3 &planeNormal, double *outT)
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
			*outT = t;
			return true;
		}
	}

	return false;
}

bool MathUtils::rayTriangleIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &v0, const Double3 &v1, const Double3 &v2, double *outT)
{
	// Möller-Trumbore
	const Double3 v0v1 = v1 - v0;
	const Double3 v0v2 = v2 - v0;
	const Double3 dirV0V2Cross = rayDirection.cross(v0v2);
	const double v0v1CrossDot = v0v1.dot(dirV0V2Cross);
	if (std::abs(v0v1CrossDot) < Constants::Epsilon)
	{
		// Ray is parallel to triangle.
		return false;
	}

	const double invDot = 1.0 / v0v1CrossDot;
	const Double3 startV0Diff = rayStart - v0;

	// First barycentric coordinate.
	const double u = invDot * startV0Diff.dot(dirV0V2Cross);
	if ((u < 0.0) || (u > 1.0))
	{
		// Outside the triangle.
		return false;
	}

	const Double3 diffV0V1Cross = startV0Diff.cross(v0v1);

	// Second barycentric coordinate.
	const double v = invDot * rayDirection.dot(diffV0V1Cross);
	if ((v < 0.0) || ((u + v) > 1.0))
	{
		// Outside the triangle.
		return false;
	}

	const double t = invDot * v0v2.dot(diffV0V1Cross);
	if (t <= Constants::Epsilon)
	{
		// Too close or the ray starts past the triangle.
		return false;
	}

	*outT = t;
	return true;
}

bool MathUtils::rayQuadIntersection(const Double3 &rayStart, const Double3 &rayDirection,
	const Double3 &v0, const Double3 &v1, const Double3 &v2, double *outT)
{
	const Double3 v3 = v0 + (v2 - v1);
	const Double3 quadNormal = (v2 - v0).cross(v1 - v0).normalized();

	double hitT;
	if (!MathUtils::rayPlaneIntersection(rayStart, rayDirection, v0, quadNormal, &hitT))
	{
		return false;
	}

	// The plane intersection is a point co-planar with the quad. Check if the point is
	// within the bounds of the quad.
	const Double3 hitPoint = rayStart + (rayDirection * hitT);
	const Double3 a = (v1 - v0).cross(hitPoint - v0);
	const Double3 b = (v2 - v1).cross(hitPoint - v1);
	const Double3 c = (v3 - v2).cross(hitPoint - v2);
	const Double3 d = (v0 - v3).cross(hitPoint - v3);
	const double ab = a.dot(b);
	const double bc = b.dot(c);
	const double cd = c.dot(d);	
	if (((ab * bc) >= 0.0) && ((bc * cd) >= 0.0))
	{
		*outT = hitT;
		return true;
	}
	else
	{
		return false;
	}
}

bool MathUtils::rayBoxIntersection(const Double3 &rayStart, const Double3 &rayDirection, const Double3 &boxCenter,
	double width, double height, double depth, Radians yRotation, double *outT)
{
	const double halfWidth = width * 0.50;
	const double halfHeight = height * 0.50;
	const double halfDepth = depth * 0.50;

	auto makeModelSpaceVertex = [yRotation](double x, double y, double z)
	{
		return Double3(
			(x * std::cos(yRotation)) - (z * std::sin(yRotation)),
			y,
			(x * std::sin(yRotation)) + (z * std::cos(yRotation)));
	};
	
	const Double3 modelVertices[] =
	{
		makeModelSpaceVertex(-halfWidth, -halfHeight, -halfDepth), // 0 0 0
		makeModelSpaceVertex(halfWidth, -halfHeight, -halfDepth), // 1 0 0
		makeModelSpaceVertex(-halfWidth, halfHeight, -halfDepth), // 0 1 0
		makeModelSpaceVertex(halfWidth, halfHeight, -halfDepth), // 1 1 0
		makeModelSpaceVertex(-halfWidth, -halfHeight, halfDepth), // 0 0 1
		makeModelSpaceVertex(halfWidth, -halfHeight, halfDepth), // 1 0 1
		makeModelSpaceVertex(-halfWidth, halfHeight, halfDepth), // 0 1 1
		makeModelSpaceVertex(halfWidth, halfHeight, halfDepth) // 1 1 1
	};

	constexpr int modelIndices[][3] =
	{
		{ 2, 0, 4 }, // -X
		{ 7, 5, 1 }, // +X
		{ 0, 1, 5 }, // -Y
		{ 3, 2, 6 }, // +Y
		{ 3, 1, 0 }, // -Z
		{ 6, 4, 5 }  // +Z
	};

	bool anyHit = false;
	double maxT = std::numeric_limits<double>::infinity();
	for (const auto &faceIndices : modelIndices)
	{
		const Double3 v0 = boxCenter + modelVertices[faceIndices[0]];
		const Double3 v1 = boxCenter + modelVertices[faceIndices[1]];
		const Double3 v2 = boxCenter + modelVertices[faceIndices[2]];

		double currentT;
		if (MathUtils::rayQuadIntersection(rayStart, rayDirection, v0, v1, v2, &currentT))
		{
			if (currentT < maxT)
			{
				anyHit = true;
				maxT = currentT;
			}
		}
	}

	if (anyHit)
	{
		*outT = maxT;
		return true;
	}
	else
	{
		return false;
	}
}

double MathUtils::distanceToPlane(const Double3 &point, const Double3 &planePoint, const Double3 &planeNormal)
{
	return point.dot(planeNormal) - planePoint.dot(planeNormal);
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
		points.emplace_back(Int2(pointX, pointY));

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

Int2 MathUtils::getZOrderCurvePoint(int index)
{
	DebugAssert(index >= 0);
	const int relevantBitCount = Bytes::findHighestSetBitIndex(index) + 1;
	int x = 0;
	int y = 0;
	for (int i = 0; i < relevantBitCount; i++)
	{
		const int dstBitIndex = i / 2;
		const bool bit = ((index >> i) & 1) != 0;
		const int bitValue = bit ? (1 << dstBitIndex) : 0;
		if ((i & 1) == 0)
		{
			x |= bitValue;
		}
		else
		{
			y |= bitValue;
		}
	}

	return Int2(x, y);
}
