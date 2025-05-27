#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <string_view>
#include <vector>

#include "TextRenderUtils.h"
#include "Texture.h"
#include "../Math/Rect.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Color.h"

class FontLibrary;
class Renderer;

enum class TextAlignment;

struct TextBoxProperties
{
	int fontDefIndex; // Index in font library.
	TextRenderTextureGenInfo textureGenInfo; // Texture dimensions, etc..
	Color defaultColor; // Color of text unless overridden.
	TextAlignment alignment;
	std::optional<TextRenderShadowInfo> shadowInfo;
	int lineSpacing; // Pixels between each line of text.

	TextBoxProperties(int fontDefIndex, const TextRenderTextureGenInfo &textureGenInfo, const Color &defaultColor,
		TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadowInfo = std::nullopt, int lineSpacing = 0);
	TextBoxProperties();
};

// Helper struct for conveniently defining Rect + Properties together since currently they are somewhat coupled
// (rect dimensions == texture dimensions). Intended for static text where the text box dimensions should be known
// at construction time. Dynamic text boxes for player input (like the player name in character creation) might not
// use init info.
struct TextBoxInitInfo
{
	Rect rect;
	TextBoxProperties properties;

	void init(const Rect &rect, TextBoxProperties &&properties);

	static TextBoxInitInfo makeWithCenter(const std::string_view text, const Int2 &center, const std::string &fontName,
		const Color &textColor, TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadow,
		int lineSpacing, const FontLibrary &fontLibrary);
	static TextBoxInitInfo makeWithCenter(const std::string_view text, const Int2 &center, const std::string &fontName,
		const Color &textColor, TextAlignment alignment, const FontLibrary &fontLibrary);
	static TextBoxInitInfo makeWithXY(const std::string_view text, int x, int y, const std::string &fontName,
		const Color &textColor, TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadow,
		int lineSpacing, const FontLibrary &fontLibrary);
	static TextBoxInitInfo makeWithXY(const std::string_view text, int x, int y, const std::string &fontName,
		const Color &textColor, TextAlignment alignment, const FontLibrary &fontLibrary);
};

class TextBox
{
private:
	Rect rect; // Screen position and render dimensions (NOT texture dimensions).
	TextBoxProperties properties;
	std::string text;
	TextRenderColorOverrideInfo colorOverrideInfo;
	ScopedUiTextureRef textureRef; // Output texture for rendering.
	bool dirty;

	// Redraws the underlying texture for display.
	void updateTexture();
public:
	TextBox();

	bool init(const Rect &rect, const TextBoxProperties &properties, Renderer &renderer);
	bool init(const TextBoxInitInfo &initInfo, Renderer &renderer);

	// Also renders text after initialization as a convenience.
	bool init(const TextBoxInitInfo &initInfo, const std::string_view text, Renderer &renderer);

	const Rect &getRect() const;
	UiTextureID getTextureID();

	void setText(const std::string_view text);

	void addOverrideColor(int charIndex, const Color &overrideColor);
	void clearOverrideColors();
};

#endif
