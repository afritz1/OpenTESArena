#include "RenderTransform.h"

void RenderTransform::clear()
{
	this->translation = Matrix4d();
	this->rotation = Matrix4d();
	this->scale = Matrix4d();
}
