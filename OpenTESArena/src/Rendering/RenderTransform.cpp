#include "RenderTransform.h"

RenderTransform::RenderTransform()
{
	this->clear();
}

void RenderTransform::clear()
{
	this->preScaleTranslation = Double3::Zero;
	this->rotation = Matrix4d::identity();
	this->scale = Matrix4d::identity();
}
