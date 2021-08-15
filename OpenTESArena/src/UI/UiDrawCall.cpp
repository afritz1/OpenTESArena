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

void UiDrawCall::init(TextureType textureType, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
{
	this->textureType = textureType;
	this->rectFunc = rectFunc;
	this->activeFunc = activeFunc;
	this->clipRect = clipRect;
}

void UiDrawCall::initWithTexture(const TextureInfoFunc &textureFunc, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
	const std::optional<Rect> &clipRect)
{
	this->init(TextureType::Texture, rectFunc, activeFunc, clipRect);
	this->textureFunc = textureFunc;
}

void UiDrawCall::initWithTextureBuilder(const TextureBuilderFunc &textureBuilderFunc, const RectFunc &rectFunc,
	const ActiveFunc &activeFunc, const std::optional<Rect> &clipRect)
{
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
