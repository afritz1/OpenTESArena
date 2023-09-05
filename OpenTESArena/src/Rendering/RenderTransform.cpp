#include "RenderTransform.h"

RenderTransform::RenderTransform()
{
	this->clear();
}

void RenderTransform::clear()
{
	this->preScaleTranslation = Double3::Zero;
	this->rotation = Matrix4d();
	this->scale = Matrix4d();
}
