#include "UiDrawCall.h"

#include "components/debug/Debug.h"

namespace
{
	UiDrawCall::TextureFunc MakeTextureFunc(UiTextureID id)
	{
		return [id]() { return id; };
	}

	UiDrawCall::RectFunc MakeRectFunc(const Rect &rect)
	{
		return [rect]() { return rect; };
	}

	bool DefaultActiveFunc()
	{
		return true;
	}
}

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect, RenderSpace renderSpace)
	: textureFunc(textureFunc), rectFunc(rectFunc), activeFunc(activeFunc), clipRect(clipRect)
{
	DebugAssert(this->textureFunc);
	DebugAssert(this->rectFunc);
	DebugAssert(this->activeFunc);
	this->renderSpace = renderSpace;
}

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const RectFunc &rectFunc, const std::optional<Rect> &clipRect,
	RenderSpace renderSpace)
	: UiDrawCall(textureFunc, rectFunc, DefaultActiveFunc, clipRect, renderSpace) { }

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const Rect &rect, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect, RenderSpace renderSpace)
	: UiDrawCall(textureFunc, MakeRectFunc(rect), activeFunc, clipRect, renderSpace) { }

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const Rect &rect, const std::optional<Rect> &clipRect,
	RenderSpace renderSpace)
	: UiDrawCall(textureFunc, MakeRectFunc(rect), DefaultActiveFunc, clipRect, renderSpace) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect, RenderSpace renderSpace)
	: UiDrawCall(MakeTextureFunc(textureID), rectFunc, activeFunc, clipRect, renderSpace) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const RectFunc &rectFunc, const std::optional<Rect> &clipRect,
	RenderSpace renderSpace)
	: UiDrawCall(MakeTextureFunc(textureID), rectFunc, DefaultActiveFunc, clipRect, renderSpace) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const Rect &rect, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect, RenderSpace renderSpace)
	: UiDrawCall(MakeTextureFunc(textureID), MakeRectFunc(rect), activeFunc, clipRect, renderSpace) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const Rect &rect, const std::optional<Rect> &clipRect,
	RenderSpace renderSpace)
	: UiDrawCall(MakeTextureFunc(textureID), MakeRectFunc(rect), DefaultActiveFunc, clipRect, renderSpace) { }

UiTextureID UiDrawCall::getTextureID() const
{
	DebugAssert(this->isActive());
	return this->textureFunc();
}

Rect UiDrawCall::getRect() const
{
	DebugAssert(this->isActive());
	return this->rectFunc();
}

bool UiDrawCall::isActive() const
{
	return this->activeFunc();
}

const std::optional<Rect> &UiDrawCall::getClipRect() const
{
	DebugAssert(this->isActive());
	return this->clipRect;
}

RenderSpace UiDrawCall::getRenderSpace() const
{
	return this->renderSpace;
}
