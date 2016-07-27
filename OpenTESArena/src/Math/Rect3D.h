#ifndef RECT_3D_H
#define RECT_3D_H

#include "Float3.h"

// Intended for use with the OpenCL kernel, if rectangles are eventually used
// instead of triangles for rendering.

class Rect3D
{
private:
	Float3f p1, p2, p3, p4;
public:
	Rect3D(const Float3f &p1, const Float3f &p2, const Float3f &p3);
	~Rect3D();

	const Float3f &getP1() const;
	const Float3f &getP2() const;
	const Float3f &getP3() const;
	const Float3f &getP4() const;

	Float3f getNormal() const;
};

#endif
