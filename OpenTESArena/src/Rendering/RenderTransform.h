#ifndef RENDER_TRANSFORM_H
#define RENDER_TRANSFORM_H

#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"

struct RenderTransform
{
	Double3 preScaleTranslation; // For scaling around arbitrary point.
	Matrix4d rotation, scale;

	RenderTransform();

	void clear();
};

#endif
