#include "UiImage.h"

UiImage::UiImage()
{
	this->textureID = -1;
}

void UiImage::init(UiTextureID textureID)
{
	this->textureID = textureID;
}
