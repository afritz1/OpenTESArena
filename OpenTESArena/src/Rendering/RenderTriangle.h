#ifndef RENDER_TRIANGLE_H
#define RENDER_TRIANGLE_H

#include <cstdint>

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Convenience class, not intended for performance. Vertex winding order is counter-clockwise.

struct RenderTriangle
{
	Double3 v0, v1, v2;
	Double3 v0v1, v1v2, v2v0;
	Double3 normal;
	Double2 uv0, uv1, uv2;
	uint32_t color;

	RenderTriangle(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double2 &uv0,
		const Double2 &uv1, const Double2 &uv2, uint32_t color);
	RenderTriangle();

	void init(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double2 &uv0,
		const Double2 &uv1, const Double2 &uv2, uint32_t color);
};

#endif
