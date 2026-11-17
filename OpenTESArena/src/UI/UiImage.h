#pragma once

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Rendering/RenderTextureUtils.h"

struct UiImage
{
	UiTextureID textureID; // Owned by UI manager.

	UiImage();

	void init(UiTextureID textureID);
};

struct UiImageSlicedInitInfo
{
	TextureAsset textureAsset; // Must be 9 textures.
	TextureAsset paletteAsset;
	std::string contentElementName;
	int paddingWidth; // Extra pixels added to left and right sides.
	int paddingHeight; // Extra pixels added to top and bottom sides.

	UiImageSlicedInitInfo();
};

struct UiImageSliced
{
	static constexpr int TEXTURE_COUNT = 9;

	UiTextureID textureID; // Owned by this image, must be freed.
	TextureBuilderID textureBuilderIDs[TEXTURE_COUNT];
	PaletteID paletteID;
	std::string contentElementName;
	int paddingWidth;
	int paddingHeight;
	bool dirty;

	UiImageSliced();

	void init(UiTextureID textureID, Span<const TextureBuilderID> textureBuilderIDs, PaletteID paletteID,
		const std::string &contentElementName, int paddingWidth, int paddingHeight);

	void free(Renderer &renderer);
};
