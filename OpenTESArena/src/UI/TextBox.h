#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <vector>

#include "TextRenderUtils.h"
#include "Texture.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"

class FontLibrary;
class Renderer;

enum class TextAlignment;

class TextBox
{
public:
	struct Properties
	{
		TextRenderUtils::TextureGenInfo textureGenInfo; // Texture dimensions, etc..
		int fontDefIndex; // Index in font library.
		Color defaultColor; // Color of text unless overridden.
		TextAlignment alignment;
		std::optional<TextRenderUtils::TextShadowInfo> shadowInfo;
		int lineSpacing; // Pixels between each line of text.

		Properties(const TextRenderUtils::TextureGenInfo &textureGenInfo, int fontDefIndex, const Color &defaultColor,
			TextAlignment alignment, const std::optional<TextRenderUtils::TextShadowInfo> &shadowInfo, int lineSpacing);
		Properties();
	};
private:
	TextRenderUtils::ColorOverrideInfo colorOverrideInfo;
	std::string text;
	Properties properties;
	Rect rect; // Screen position and render dimensions (NOT texture dimensions).
	Texture texture; // Output texture for rendering.
	bool dirty;
public:
	TextBox();

	void init(const Rect &rect, const Properties &properties, Renderer &renderer);

	const Rect &getRect() const;
	const Texture &getTexture() const;

	void setText(std::string &&text);

	void addOverrideColor(int textIndex, const Color &overrideColor);
	void clearOverrideColors();

	// Redraws the underlying texture for display.
	void updateTexture(const FontLibrary &fontLibrary);
};

#endif
