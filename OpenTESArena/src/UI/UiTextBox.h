#pragma once

#include <optional>
#include <string>

#include "TextRenderUtils.h"
#include "../Assets/TextureUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Color.h"

class Renderer;

enum class TextAlignment;

struct UiTextBoxInitInfo
{
	std::string worstCaseText; // Determines texture dimensions if actual text is empty (intended for frequently modified text boxes).
	std::string text; // Actual text for presentation.
	std::string fontName;
	Color defaultColor;
	PaletteID tabColorPaletteID;
	TextAlignment alignment;
	std::optional<TextRenderShadowInfo> shadowInfo;
	int lineSpacing;

	UiTextBoxInitInfo();
};

struct UiTextBox
{
	std::string text;
	bool dirty;

	UiTextureID textureID; // Owned by this text box, must be freed.
	int textureWidth;
	int textureHeight;

	int fontDefIndex; // Index in font library.
	Color defaultColor;
	PaletteID tabColorPaletteID; // Provides a palette for tab color ('\t') lookups.
	TextAlignment alignment;
	std::optional<TextRenderShadowInfo> shadowInfo;
	int lineSpacing; // Pixels between each line of text.

	UiTextBox();

	void init(UiTextureID textureID, int textureWidth, int textureHeight, int fontDefIndex, const Color &defaultColor,
		PaletteID tabColorPaletteID, TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadowInfo, int lineSpacing);

	void free(Renderer &renderer);
};
