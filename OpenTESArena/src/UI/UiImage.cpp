#include <algorithm>

#include "UiImage.h"
#include "../Rendering/Renderer.h"

UiImage::UiImage()
{
	this->textureID = -1;
}

void UiImage::init(UiTextureID textureID)
{
	this->textureID = textureID;
}

UiImageSlicedInitInfo::UiImageSlicedInitInfo()
{
	this->paddingWidth = 0;
	this->paddingHeight = 0;	
}

UiImageSliced::UiImageSliced()
{
	this->textureID = -1;
	std::fill(std::begin(this->textureBuilderIDs), std::end(this->textureBuilderIDs), -1);
	this->paletteID = -1;
	this->paddingWidth = 0;
	this->paddingHeight = 0;
	this->dirty = false;
}

void UiImageSliced::init(UiTextureID textureID, Span<const TextureBuilderID> textureBuilderIDs, PaletteID paletteID,
	const std::string &contentElementName, int paddingWidth, int paddingHeight)
{
	DebugAssert(textureBuilderIDs.getCount() == std::size(this->textureBuilderIDs));
	this->textureID = textureID;
	std::copy(textureBuilderIDs.begin(), textureBuilderIDs.end(), std::begin(this->textureBuilderIDs));
	this->paletteID = paletteID;
	this->contentElementName = contentElementName;
	this->paddingWidth = paddingWidth;
	this->paddingHeight = paddingHeight;
	this->dirty = true;
}

void UiImageSliced::free(Renderer &renderer)
{
	if (this->textureID >= 0)
	{
		renderer.freeUiTexture(this->textureID);
		this->textureID = -1;
	}
}
