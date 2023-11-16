#ifndef RENDER_TRANSFORM_H
#define RENDER_TRANSFORM_H

#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"

struct RenderTransform
{
	Matrix4d translation, rotation, scale; // @todo: this should just be a model transform (T*R*S result) once the renderer is using vertex shaders w/ MVP multiplication

	RenderTransform();

	void clear();
};

#endif
