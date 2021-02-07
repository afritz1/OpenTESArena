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

void RendererSystem2D::draw(UiTextureID id, double x, double y, double width, double height,
	RenderSpace renderSpace)
{
	const RenderElement element(id, x, y, width, height);
	this->draw(&element, 1, renderSpace);
}

void RendererSystem2D::draw(UiTextureID id, double x, double y, RenderSpace renderSpace)
{
	const std::optional<Int2> dims = this->tryGetTextureDims(id);
	if (!dims.has_value())
	{
		DebugLogError("Couldn't get dimensions for UI texture \"" + std::to_string(id) + "\".");
		return;
	}

	this->draw(id, x, y, dims->x, dims->y, renderSpace);
}
