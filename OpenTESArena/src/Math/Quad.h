#ifndef QUAD_H
#define QUAD_H

#include "Vector3.h"

// Convenience class, not intended for performance.

// Vertices are ordered in a loop around the quad (i.e. v0: top left, v1: bottom left,
// v2: bottom right, etc.).

class Quad
{
private:
	Double3 v0, v1, v2, v3;
public:
	Quad();
	Quad(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double3 &v3);

	// Implicitly constructs the fourth corner.
	Quad(const Double3 &v0, const Double3 &v1, const Double3 &v2);

	void init(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double3 &v3);
	void init(const Double3 &v0, const Double3 &v1, const Double3 &v2);

	// Gets individual corners of the quad.
	const Double3 &getV0() const;
	const Double3 &getV1() const;
	const Double3 &getV2() const;
	const Double3 &getV3() const;

	// Vectors between various quad edges.
	Double3 getV0V1() const;
	Double3 getV1V2() const;
	Double3 getV2V3() const;
	Double3 getV3V0() const;

	// Gets the normal of the quad.
	Double3 getNormal() const;
};

#endif