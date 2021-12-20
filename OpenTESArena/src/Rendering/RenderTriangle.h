#ifndef RENDER_TRIANGLE_H
#define RENDER_TRIANGLE_H

#include "../Math/Vector3.h"

// Convenience class, not intended for performance. Vertex winding order is counter-clockwise.

struct RenderTriangle
{
	Double3 v0, v1, v2;
	Double3 v0v1, v1v2, v2v0;
	Double3 normal;

	RenderTriangle(const Double3 &v0, const Double3 &v1, const Double3 &v2);
	RenderTriangle();

	void init(const Double3 &v0, const Double3 &v1, const Double3 &v2);
};

#endif
