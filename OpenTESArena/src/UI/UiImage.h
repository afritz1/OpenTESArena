#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "../Rendering/RenderTextureUtils.h"

struct UiImage
{
	UiTextureID textureID; // Owned by UI manager.
	// @todo clip rect (valid if dims not 0)

	UiImage();

	void init(UiTextureID textureID);
};

#endif
