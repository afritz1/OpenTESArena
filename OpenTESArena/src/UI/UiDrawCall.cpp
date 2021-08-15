#include "UiDrawCall.h"

#include "components/debug/Debug.h"

UiDrawCall::TextureInfo::TextureInfo(const Texture &texture)
	: texture(&texture) { }

UiDrawCall::TextureBuilderInfo::TextureBuilderInfo(TextureBuilderID textureBuilderID, PaletteID paletteID)
{
	this->textureBuilderID = textureBuilderID;
	this->paletteID = paletteID;
}

UiDrawCall::UiDrawCall()
{
	this->textureType = static_cast<TextureType>(-1);
}

UiDrawCall::TextureBuilderFunc UiDrawCall::makeTextureBuilderFunc(TextureBuilderID textureBuilderID, PaletteID paletteID)
{
	return [textureBuilderID, paletteID]()
	{
		return UiDrawCall::TextureBuilderInfo(textureBuilderID, paletteID);
	};
}

UiDrawCall::RectFunc UiDrawCall::makeRectFunc(const Rect &rect)
{
	return [rect]() { return rect; };
}

bool UiDrawCall::defaultActiveFunc()
{
	return true;
}

void UiDrawCall::init(TextureType textureType, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
{
	DebugAssert(rectFunc);
	DebugAssert(activeFunc);

	this->textureType = textureType;
	this->rectFunc = rectFunc;
	this->activeFunc = activeFunc;
	this->clipRect = clipRect;
}

void UiDrawCall::initWithTexture(const TextureFunc &textureFunc, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
{
	DebugAssert(textureFunc);
	this->init(TextureType::Texture, rectFunc, activeFunc, clipRect);
	this->textureFunc = textureFunc;
}

void UiDrawCall::initWithTextureBuilder(const TextureBuilderFunc &textureBuilderFunc, const RectFunc &rectFunc,
	const ActiveFunc &activeFunc, const std::optional<Rect> &clipRect)
{
	DebugAssert(textureBuilderFunc);
	this->init(TextureType::TextureBuilder, rectFunc, activeFunc, clipRect);
	this->textureBuilderFunc = textureBuilderFunc;
}

UiDrawCall::TextureType UiDrawCall::getTextureType() const
{
	return this->textureType;
}

UiDrawCall::TextureInfo UiDrawCall::getTextureInfo() const
{
	DebugAssert(this->textureType == TextureType::Texture);
	return this->textureFunc();
}

UiDrawCall::TextureBuilderInfo UiDrawCall::getTextureBuilderInfo() const
{
	DebugAssert(this->textureType == TextureType::TextureBuilder);
	return this->textureBuilderFunc();
}

Rect UiDrawCall::getRect() const
{
	return this->rectFunc();
}

bool UiDrawCall::isActive() const
{
	return this->activeFunc();
}

const std::optional<Rect> &UiDrawCall::getClipRect() const
{
	return this->clipRect;
}
