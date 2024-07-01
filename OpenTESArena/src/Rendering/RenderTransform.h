#ifndef RENDER_TRANSFORM_H
#define RENDER_TRANSFORM_H

#include "../Math/Matrix4.h"

struct RenderTransform
{
	Matrix4d translation, rotation, scale;

	void clear();
};

#endif
