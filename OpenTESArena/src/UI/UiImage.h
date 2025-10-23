#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "../Rendering/RenderTextureUtils.h"

using UiImageInstanceID = int;

struct UiImage
{
	UiTextureID textureID; // Owned by UI manager.

	UiImage();

	void init(UiTextureID textureID);
};

#endif
