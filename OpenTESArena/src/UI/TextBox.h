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
		int fontDefIndex; // Index in font library.
		TextRenderUtils::TextureGenInfo textureGenInfo; // Texture dimensions, etc..
		Color defaultColor; // Color of text unless overridden.
		TextAlignment alignment;
		std::optional<TextRenderUtils::TextShadowInfo> shadowInfo;
		int lineSpacing; // Pixels between each line of text.

		Properties(int fontDefIndex, const TextRenderUtils::TextureGenInfo &textureGenInfo, const Color &defaultColor,
			TextAlignment alignment, const std::optional<TextRenderUtils::TextShadowInfo> &shadowInfo, int lineSpacing);
		Properties();
	};
private:
	Rect rect; // Screen position and render dimensions (NOT texture dimensions).
	Properties properties;
	std::string text;
	TextRenderUtils::ColorOverrideInfo colorOverrideInfo;
	Texture texture; // Output texture for rendering.
	bool dirty;
public:
	TextBox();

	bool init(const Rect &rect, const Properties &properties, Renderer &renderer);

	const Rect &getRect() const;
	const Texture &getTexture() const;

	void setText(std::string &&text);

	void addOverrideColor(int textIndex, const Color &overrideColor);
	void clearOverrideColors();

	// Redraws the underlying texture for display.
	void updateTexture(const FontLibrary &fontLibrary);
};

#endif
