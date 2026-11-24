#pragma once

#include <functional>
#include <optional>
#include <string>

#include "TextRenderUtils.h"
#include "../Assets/TextureUtils.h"
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

static constexpr int UI_MESSAGE_BOX_MAX_BUTTONS = 8;

using UiMessageBoxButtonCallback = UiButtonCallback;

struct UiMessageBoxInitInfo
{
	std::string titleText;
	std::string titleFontName;
	Color titleTextColor;
	int titleMinWidth;
	int titleMinHeight;
	int titleLineSpacing;

	std::string buttonTexts[UI_MESSAGE_BOX_MAX_BUTTONS];
	UiMessageBoxButtonCallback buttonCallbacks[UI_MESSAGE_BOX_MAX_BUTTONS];
	std::string buttonFontName;
	Color buttonTextColor;
	int buttonMinHeight;
	int buttonCount;

	UiTexturePatternType patternType;
	int paddingWidth;
	int paddingHeight;

	UiMessageBoxInitInfo();

	void addButton(const std::string &text, const UiMessageBoxButtonCallback &callback);
};

struct UiMessageBox
{
	UiTextureID textureID; // Owned by this message box, must be freed.
	PaletteID paletteID; // For tab color text.

	std::string titleText;
	int titleFontDefIndex;
	Color titleTextColor;
	int titleMinWidth;
	int titleMinHeight;
	int titleLineSpacing;

	std::string buttonTexts[UI_MESSAGE_BOX_MAX_BUTTONS];
	UiMessageBoxButtonCallback buttonCallbacks[UI_MESSAGE_BOX_MAX_BUTTONS];
	int buttonFontDefIndex;
	Color buttonTextColor;
	int buttonMinHeight;
	int buttonCount;

	UiTexturePatternType patternType;
	int paddingWidth; // Extra pixels added to left and right sides.
	int paddingHeight; // Extra pixels added to top and bottom sides.

	bool dirty;

	UiMessageBox();

	void init(UiTextureID textureID, PaletteID paletteID, const std::string &titleText, int titleFontDefIndex, Color titleTextColor,
		int titleMinWidth, int titleMinHeight, int titleLineSpacing, Span<const std::string> buttonTexts,
		Span<const UiMessageBoxButtonCallback> buttonCallbacks, int buttonFontDefIndex, Color buttonTextColor, int buttonMinHeight,
		int buttonCount, UiTexturePatternType patternType, int paddingWidth, int paddingHeight);

	void free(Renderer &renderer);
};
