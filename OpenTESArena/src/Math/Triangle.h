#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Vector2.h"
#include "Vector3.h"

// Use doubles here, and convert them to floats when sending to the OpenCL kernel.
// If it's a bottleneck, then store them as floats here instead.

// The normal is calculated as needed since it probably isn't going to be a bottleneck.
// Both it and the two tangent vectors are only stored in the OpenCL kernel for 
// performance.

class Triangle
{
private:
	Vector3d p1, p2, p3;
	Vector2d uv1, uv2, uv3;
public:
	// Initializes a triangle with three points and three texture coordinates.
	Triangle(const Vector3d &p1, const Vector3d &p2, const Vector3d &p3,
		const Vector2d &uv1, const Vector2d &uv2, const Vector2d &uv3);
	~Triangle();

	const Vector3d &getP1() const;
	const Vector3d &getP2() const;
	const Vector3d &getP3() const;
	const Vector2d &getUV1() const;
	const Vector2d &getUV2() const;
	const Vector2d &getUV3() const;

	// These three normal methods could return const& if they were triangle members, 
	// but it's unnecessary space to have them all stored on the host when they're 
	// only going to be used in the OpenCL kernel. The UV coordinates are members 
	// because they can't be calculated alone.
	Vector3d getNormal() const;
	Vector3d getTangent() const;
	Vector3d getBitangent() const;
};

#endif