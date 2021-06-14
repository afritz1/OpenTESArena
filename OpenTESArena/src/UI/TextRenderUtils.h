#ifndef TEXT_RENDER_UTILS_H
#define TEXT_RENDER_UTILS_H

#include <cstdint>
#include <string_view>
#include <vector>

#include "FontDefinition.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView2D.h"

namespace TextRenderUtils
{
	// Data for generating a text box texture.
	struct TextureGenInfo
	{
		int width, height; // In pixels.
		// @todo: any other cached info for performance

		TextureGenInfo();

		void init(int width, int height);
	};

	// Splits a string of text into lines based on newline characters.
	std::vector<std::string_view> getTextLines(const std::string_view &text);

	// Gets the font characters needed to render each character in the given line of text.
	std::vector<FontDefinition::CharID> getLineFontCharIDs(const std::string_view &line, const FontDefinition &fontDef);

	// Determines how large a text box texture should be in pixels.
	TextureGenInfo makeTextureGenInfo(const std::string_view &text, const FontDefinition &fontDef, int lineSpacing);
	TextureGenInfo makeTextureGenInfo(const std::string_view &text, const FontDefinition &fontDef);

	// Blits the given font character to the output texture, and handles clipping.
	// @todo: this should draw to a UI texture via UiTextureID eventually. Process will be:
	// - calculate texture width and height based on text, font, line spacing
	// - allocate UI texture
	// - draw text
	// - render
	void drawChar(const FontDefinition::Character &fontChar, int dstX, int dstY, const Color &textColor,
		BufferView2D<uint32_t> &outBuffer);
	void drawTextLine(const BufferView<FontDefinition::CharID> &charIDs, const FontDefinition &fontDef,
		int dstX, int dstY, const Color &textColor, BufferView2D<uint32_t> &outBuffer);
}

#endif
