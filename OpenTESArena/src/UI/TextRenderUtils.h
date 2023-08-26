#ifndef TEXT_RENDER_UTILS_H
#define TEXT_RENDER_UTILS_H

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "FontDefinition.h"
#include "../Math/Vector2.h"
#include "../Utilities/Color.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView2D.h"

enum class TextAlignment;

namespace TextRenderUtils
{
	// Used when determining worst-case text box dimensions.
	constexpr char LARGEST_CHAR = 'W';

	// Data for generating a texture for rendering text into.
	struct TextureGenInfo
	{
		int width, height; // In pixels.
		// @todo: any other cached info for performance

		TextureGenInfo();

		void init(int width, int height);
	};

	// Data for replacing default text character colors with overrides.
	class ColorOverrideInfo
	{
	public:
		struct Entry
		{
			int charIndex; // Index of character in text.
			Color color;

			Entry(int charIndex, const Color &color);
		};
	private:
		std::vector<Entry> entries;
	public:
		// Generates a list of color override entries from text containing the "tab-color" pattern,
		// where an 8-bit palette index follows a '\t' character.
		static std::vector<Entry> makeEntriesFromText(const std::string_view &text, const Palette &palette);

		int getEntryCount() const;
		std::optional<int> findEntryIndex(int charIndex) const;
		const Color &getColor(int entryIndex) const;

		void add(int charIndex, const Color &color);
		void clear();
	};

	// Data for positioning a shadow within a text box.
	struct TextShadowInfo
	{
		int offsetX, offsetY;
		Color color;

		TextShadowInfo(int offsetX, int offsetY, const Color &color);
		TextShadowInfo();

		void init(int offsetX, int offsetY, const Color &color);
	};

	// Splits a string of text into lines based on newline characters.
	Buffer<std::string_view> getTextLines(const std::string_view &text);

	// Gets the font characters needed to render each character in the given line of text.
	Buffer<FontDefinition::CharID> getLineFontCharIDs(const std::string_view &line, const FontDefinition &fontDef);

	// Gets the number of pixels long a rendered line of characters would be.
	int getLinePixelWidth(BufferView<const FontDefinition::CharID> charIDs, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow = std::nullopt);
	int getLinePixelWidth(const std::string_view &line, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow = std::nullopt);

	// Gets the number of pixels wide or tall a rendered block of text lines would be.
	int getLinesPixelWidth(BufferView<const std::string_view> textLines, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow = std::nullopt);
	int getLinesPixelHeight(BufferView<const std::string_view> textLines, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow = std::nullopt, int lineSpacing = 0);

	// Determines how large a text box texture should be in pixels.
	// @todo: might need to change lineSpacing to a percent of character height so it scales with HD fonts
	TextureGenInfo makeTextureGenInfo(BufferView<const std::string_view> textLines, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow = std::nullopt, int lineSpacing = 0);
	TextureGenInfo makeTextureGenInfo(const std::string_view &text, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow = std::nullopt, int lineSpacing = 0);

	// Generates XY pixel offsets for each line of a text box based on text alignment.
	Buffer<Int2> makeAlignmentOffsets(BufferView<const std::string_view> textLines, int textureWidth,
		int textureHeight, TextAlignment alignment, const FontDefinition &fontDef,
		const std::optional<TextShadowInfo> &shadow, int lineSpacing);

	// Blits the given font character to the output texture, and handles clipping.
	// @todo: this should draw to a UI texture via UiTextureID eventually. Process will be:
	// - calculate texture width and height based on text, font, line spacing
	// - allocate UI texture
	// - draw text
	// - render
	void drawChar(const FontDefinition::Character &fontChar, int dstX, int dstY, const Color &textColor,
		BufferView2D<uint32_t> &outBuffer);
	void drawTextLine(BufferView<const FontDefinition::CharID> charIDs, const FontDefinition &fontDef,
		int dstX, int dstY, const Color &textColor, const ColorOverrideInfo *colorOverrideInfo, const TextShadowInfo *shadow,
		BufferView2D<uint32_t> &outBuffer);
	void drawTextLine(const std::string_view &line, const FontDefinition &fontDef, int dstX, int dstY,
		const Color &textColor, const ColorOverrideInfo *colorOverrideInfo, const TextShadowInfo *shadow,
		BufferView2D<uint32_t> &outBuffer);
	void drawTextLines(BufferView<const std::string_view> textLines, const FontDefinition &fontDef, int dstX, int dstY,
		const Color &textColor, TextAlignment alignment, int lineSpacing, const ColorOverrideInfo *colorOverrideInfo,
		const TextShadowInfo *shadow, BufferView2D<uint32_t> &outBuffer);
}

#endif
