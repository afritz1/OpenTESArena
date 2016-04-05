#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Float2.h"
#include "Float3.h"

// Use doubles here, and convert them to floats when sending to the OpenCL kernel.
// If it's a bottleneck, then store them as floats here instead.

// The normal is calculated as needed since it probably isn't going to be a bottleneck.
// Both it and the two tangent vectors are only stored in the OpenCL kernel for 
// performance.

class Triangle
{
private:
	Float3d p1, p2, p3;
	Float2d uv1, uv2, uv3;
public:
	// Maybe have a constructor for floats and a constructor for doubls. They
	// should be stored in the triangle as Float3f and Float2f since they're only
	// going to be used for rendering in the OpenCL kernel.

	// Initializes a triangle with three points and three texture coordinates.
	Triangle(const Float3d &p1, const Float3d &p2, const Float3d &p3,
		const Float2d &uv1, const Float2d &uv2, const Float2d &uv3);
	~Triangle();

	const Float3d &getP1() const;
	const Float3d &getP2() const;
	const Float3d &getP3() const;
	const Float2d &getUV1() const;
	const Float2d &getUV2() const;
	const Float2d &getUV3() const;

	// These three normal methods could return const& if they were triangle members, 
	// but it's unnecessary space to have them all stored on the host when they're 
	// only going to be used in the OpenCL kernel. The UV coordinates are members 
	// because they can't be calculated alone.
	Float3d getNormal() const;
	Float3d getTangent() const;
	Float3d getBitangent() const;
};

#endif