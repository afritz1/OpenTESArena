#ifndef MESSAGE_BOX_SUB_PANEL_H
#define MESSAGE_BOX_SUB_PANEL_H

#include <optional>
#include <string>

#include "../Assets/TextureUtils.h"
#include "../UI/TextRenderUtils.h"
#include "../Utilities/Color.h"

struct MessageBoxBackgroundProperties
{
	UiTexturePatternType patternType;
	int extraTitleWidth, extraTitleHeight;
	std::optional<int> widthOverride, heightOverride; // In case the texture is independent of the title text.
	int itemTextureHeight; // Width is driven by title background texture.

	MessageBoxBackgroundProperties(UiTexturePatternType patternType, int extraTitleWidth, int extraTitleHeight,
		const std::optional<int> &widthOverride, const std::optional<int> &heightOverride, int itemTextureHeight);
};

struct MessageBoxTitleProperties
{
	std::string fontName;
	TextRenderTextureGenInfo textureGenInfo; // Texture dimensions, etc..
	Color textColor;
	int lineSpacing;

	MessageBoxTitleProperties(const std::string &fontName, const TextRenderTextureGenInfo &textureGenInfo,
		const Color &textColor, int lineSpacing = 0);
};

struct MessageBoxItemsProperties
{
	int count;
	std::string fontName;
	TextRenderTextureGenInfo textureGenInfo; // Texture dimensions, etc..
	Color textColor;

	MessageBoxItemsProperties(int count, const std::string &fontName, const TextRenderTextureGenInfo &textureGenInfo,
		const Color &textColor);
};

#endif
