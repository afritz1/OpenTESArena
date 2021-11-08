#include "RendererSystem2D.h"

#include "components/debug/Debug.h"

RendererSystem2D::RenderElement::RenderElement(UiTextureID id, double x, double y, double width, double height)
{
	this->id = id;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}

RendererSystem2D::~RendererSystem2D()
{
	// Do nothing.
}
