#pragma once

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
#include "components/utilities/Span.h"
#include "components/utilities/Span2D.h"

enum class TextAlignment;

// Data for generating a texture for rendering text into.
struct TextRenderTextureGenInfo
{
	int width, height; // In pixels.
	// @todo: any other cached info for performance

	TextRenderTextureGenInfo();

	void init(int width, int height);
};

// Data for positioning a shadow within a text box.
struct TextRenderShadowInfo
{
	int offsetX, offsetY;
	Color color;

	TextRenderShadowInfo(int offsetX, int offsetY, Color color);
	TextRenderShadowInfo();

	void init(int offsetX, int offsetY, Color color);
};

// An index in the charIDs span that changes the current text rendering color. Internal to text rendering code.
struct TextRenderTabColorOverrideEntry
{
	int charIdIndex;
	Color color;

	TextRenderTabColorOverrideEntry();
};

namespace TextRenderUtils
{
	// Used when determining worst-case text box dimensions.
	constexpr char LARGEST_CHAR = 'W';

	// Makes a simple string of W's with no newlines.
	std::string makeWorstCaseText(int charCount);

	// Splits a string of text into lines based on newline characters.
	Buffer<std::string_view> getTextLines(const std::string_view text);

	// Gets the font characters needed to render each character in the given line of text.
	Buffer<FontDefinitionCharacterID> getLineFontCharIDs(const std::string_view line, const FontDefinition &fontDef);

	// Gets any tab color overrides from the text defined by a '\t' followed by an 8-bit palette index.
	std::vector<TextRenderTabColorOverrideEntry> getLineTabColorOverrideEntries(const std::string_view line, const Palette *palette);

	// Gets the number of pixels long a rendered line of characters would be.
	int getLinePixelWidth(Span<const FontDefinitionCharacterID> charIDs, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow = std::nullopt);
	int getLinePixelWidth(const std::string_view line, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow = std::nullopt);

	// Gets the number of pixels wide or tall a rendered block of text lines would be.
	int getLinesPixelWidth(Span<const std::string_view> textLines, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow = std::nullopt);
	int getLinesPixelHeight(Span<const std::string_view> textLines, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow = std::nullopt, int lineSpacing = 0);

	// Determines how large a text box texture should be in pixels.
	// @todo: might need to change lineSpacing to a percent of character height so it scales with HD fonts
	TextRenderTextureGenInfo makeTextureGenInfo(Span<const std::string_view> textLines, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow = std::nullopt, int lineSpacing = 0);
	TextRenderTextureGenInfo makeTextureGenInfo(const std::string_view text, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow = std::nullopt, int lineSpacing = 0);

	// Generates XY pixel offsets for each line of a text box based on text alignment.
	Buffer<Int2> makeAlignmentOffsets(Span<const std::string_view> textLines, int textureWidth,
		int textureHeight, TextAlignment alignment, const FontDefinition &fontDef,
		const std::optional<TextRenderShadowInfo> &shadow, int lineSpacing);

	// Blits the given font character to the output texture, and handles clipping.
	// @todo: this should draw to a UI texture via UiTextureID eventually. Process will be:
	// - calculate texture width and height based on text, font, line spacing
	// - allocate UI texture
	// - draw text
	// - render
	void drawChar(const FontDefinitionCharacter &fontChar, int dstX, int dstY, Color textColor, Span2D<uint32_t> outBuffer);
	void drawTextLine(Span<const FontDefinitionCharacterID> charIDs, const FontDefinition &fontDef,
		int dstX, int dstY, Color textColor, Span<const TextRenderTabColorOverrideEntry> tabColorOverrideEntries,
		const TextRenderShadowInfo *shadow, Span2D<uint32_t> outBuffer);
	void drawTextLine(const std::string_view line, const FontDefinition &fontDef, int dstX, int dstY,
		Color textColor, const Palette *tabColorPalette, const TextRenderShadowInfo *shadow, Span2D<uint32_t> outBuffer);
	void drawTextLines(Span<const std::string_view> textLines, const FontDefinition &fontDef, int dstX, int dstY,
		Color textColor, const Palette *tabColorPalette, TextAlignment alignment, int lineSpacing, const TextRenderShadowInfo *shadow,
		Span2D<uint32_t> outBuffer);
}
