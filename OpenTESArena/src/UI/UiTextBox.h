#ifndef UI_TEXT_BOX_H
#define UI_TEXT_BOX_H

#include <optional>
#include <string>

#include "TextRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Color.h"

class Renderer;

enum class TextAlignment;

struct UiTextBoxInitInfo
{
	std::string worstCaseText; // Determines texture dimensions.
	std::string text; // Actual text for presentation.
	const char *fontName;
	Color defaultColor;
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
	TextRenderColorOverrideInfo colorOverrideInfo;
	TextAlignment alignment;
	std::optional<TextRenderShadowInfo> shadowInfo;
	int lineSpacing; // Pixels between each line of text.

	UiTextBox();

	void init(UiTextureID textureID, int textureWidth, int textureHeight, int fontDefIndex, const Color &defaultColor,
		TextAlignment alignment, int lineSpacing);

	void free(Renderer &renderer);

	// @todo update texture and set dirty = false

	// @todo add/clear override color
};

#endif
