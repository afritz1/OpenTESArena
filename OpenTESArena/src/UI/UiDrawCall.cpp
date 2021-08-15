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
	const std::optional<Rect> &clipRect)
	: textureFunc(textureFunc), rectFunc(rectFunc), activeFunc(activeFunc), clipRect(clipRect)
{
	DebugAssert(this->textureFunc);
	DebugAssert(this->rectFunc);
	DebugAssert(this->activeFunc);
}

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const RectFunc &rectFunc, const std::optional<Rect> &clipRect)
	: UiDrawCall(textureFunc, rectFunc, DefaultActiveFunc, clipRect) { }

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const Rect &rect, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
	: UiDrawCall(textureFunc, MakeRectFunc(rect), activeFunc, clipRect) { }

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const Rect &rect, const std::optional<Rect> &clipRect)
	: UiDrawCall(textureFunc, MakeRectFunc(rect), DefaultActiveFunc, clipRect) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
	: UiDrawCall(MakeTextureFunc(textureID), rectFunc, activeFunc, clipRect) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const RectFunc &rectFunc, const std::optional<Rect> &clipRect)
	: UiDrawCall(MakeTextureFunc(textureID), rectFunc, DefaultActiveFunc, clipRect) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const Rect &rect, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
	: UiDrawCall(MakeTextureFunc(textureID), MakeRectFunc(rect), activeFunc, clipRect) { }

UiDrawCall::UiDrawCall(UiTextureID textureID, const Rect &rect, const std::optional<Rect> &clipRect)
	: UiDrawCall(MakeTextureFunc(textureID), MakeRectFunc(rect), DefaultActiveFunc, clipRect) { }

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
