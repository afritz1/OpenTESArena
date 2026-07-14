#include <algorithm>
#include <cmath>

#include "TextAlignment.h"
#include "TextRenderUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

TextRenderTextureGenInfo::TextRenderTextureGenInfo()
{
	this->width = 0;
	this->height = 0;
}

void TextRenderTextureGenInfo::init(int width, int height)
{
	this->width = width;
	this->height = height;
}

TextRenderShadowInfo::TextRenderShadowInfo(int offsetX, int offsetY, Color color)
	: color(color)
{
	this->offsetX = offsetX;
	this->offsetY = offsetY;
}

TextRenderShadowInfo::TextRenderShadowInfo()
{
	this->offsetX = 0;
	this->offsetY = 0;
}

void TextRenderShadowInfo::init(int offsetX, int offsetY, Color color)
{
	this->offsetX = offsetX;
	this->offsetY = offsetY;
	this->color = color;
}

TextRenderTabColorOverrideEntry::TextRenderTabColorOverrideEntry()
{
	this->charIdIndex = -1;
}

std::string TextRenderUtils::makeWorstCaseText(int charCount)
{
	return std::string(charCount, TextRenderUtils::LARGEST_CHAR);
}

Buffer<std::string_view> TextRenderUtils::getTextLines(const std::string_view text)
{
	// @todo: might eventually handle "\r\n".
	return StringView::split(text, '\n');
}

Buffer<FontDefinitionCharacterID> TextRenderUtils::getLineFontCharIDs(const std::string_view line, const FontDefinition &fontDef)
{
	const int charCount = static_cast<int>(line.size());

	Buffer<FontDefinitionCharacterID> charIDs(charCount);
	for (int i = 0; i < charCount; i++)
	{
		const char c = line[i]; // @todo: support more than ASCII
		const std::string charUtf8(1, c);
		const bool isTabColor = (i > 0) && (line[i - 1] == '\t');

		FontDefinitionCharacterID charID;
		if (isTabColor)
		{
			charID = -1;
		}
		else if (!fontDef.tryGetCharacterID(charUtf8.c_str(), &charID))
		{
			//DebugLogWarningFormat("Character \"%s\" not renderable with font %s.", charUtf8.c_str(), fontDef.name.c_str());
			charID = -1;
		}

		charIDs[i] = charID;
	}

	return charIDs;
}

std::vector<TextRenderTabColorOverrideEntry> TextRenderUtils::getLineTabColorOverrideEntries(const std::string_view line, const Palette *palette)
{
	const int charCount = static_cast<int>(line.size());

	std::vector<TextRenderTabColorOverrideEntry> tabColorOverrideEntries;
	for (int i = 0; i < charCount; i++)
	{
		const char c = line[i]; // @todo: support more than ASCII
		const std::string charUtf8(1, c);
		if ((charUtf8.size() == 1) && (charUtf8[0] == '\t') && (i < (charCount - 1)))
		{
			const uint8_t paletteIndex = static_cast<uint8_t>(line[i + 1]);
			Color paletteColor = Colors::Magenta;
			if (palette != nullptr)
			{
				paletteColor = (*palette)[paletteIndex];
			}

			TextRenderTabColorOverrideEntry entry;
			entry.charIdIndex = i + 2;
			entry.color = paletteColor;
			tabColorOverrideEntries.emplace_back(std::move(entry));
			i += 2;
		}
	}

	return tabColorOverrideEntries;
}

int TextRenderUtils::getLinePixelWidth(Span<const FontDefinitionCharacterID> charIDs, const FontDefinition &fontDef,
	const std::optional<TextRenderShadowInfo> &shadow)
{
	int width = 0;
	for (const FontDefinitionCharacterID charID : charIDs)
	{
		if (charID < 0)
		{
			continue;
		}

		const FontDefinitionCharacter &fontChar = fontDef.getCharacter(charID);
		width += fontChar.getWidth();
	}

	if (shadow.has_value())
	{
		width += std::abs(shadow->offsetX);
	}

	return width;
}

int TextRenderUtils::getLinePixelWidth(const std::string_view line, const FontDefinition &fontDef,
	const std::optional<TextRenderShadowInfo> &shadow)
{
	const Buffer<FontDefinitionCharacterID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
	return TextRenderUtils::getLinePixelWidth(charIDs, fontDef, shadow);
}

int TextRenderUtils::getLinesPixelWidth(Span<const std::string_view> textLines, const FontDefinition &fontDef,
	const std::optional<TextRenderShadowInfo> &shadow)
{
	int width = 0;
	for (const std::string_view line : textLines)
	{
		const int linePixelWidth = TextRenderUtils::getLinePixelWidth(line, fontDef, shadow);
		width = std::max(width, linePixelWidth);
	}

	return width;
}

int TextRenderUtils::getLinesPixelHeight(Span<const std::string_view> textLines, const FontDefinition &fontDef,
	const std::optional<TextRenderShadowInfo> &shadow, int lineSpacing)
{
	const int lineCount = textLines.getCount();
	return (fontDef.characterHeight * lineCount) + (lineSpacing * std::max(0, lineCount - 1)) + (shadow.has_value() ? std::abs(shadow->offsetY) : 0);
}

TextRenderTextureGenInfo TextRenderUtils::makeTextureGenInfo(Span<const std::string_view> textLines,
	const FontDefinition &fontDef, const std::optional<TextRenderShadowInfo> &shadow, int lineSpacing)
{
	const int width = TextRenderUtils::getLinesPixelWidth(textLines, fontDef, shadow);
	const int height = TextRenderUtils::getLinesPixelHeight(textLines, fontDef, shadow, lineSpacing);

	TextRenderTextureGenInfo textureGenInfo;
	textureGenInfo.init(width, height);
	return textureGenInfo;
}

TextRenderTextureGenInfo TextRenderUtils::makeTextureGenInfo(const std::string_view text,
	const FontDefinition &fontDef, const std::optional<TextRenderShadowInfo> &shadow, int lineSpacing)
{
	const Buffer<std::string_view> textLines = TextRenderUtils::getTextLines(text);
	return TextRenderUtils::makeTextureGenInfo(textLines, fontDef, shadow, lineSpacing);
}

Buffer<Int2> TextRenderUtils::makeAlignmentOffsets(Span<const std::string_view> textLines,
	int textureWidth, int textureHeight, TextAlignment alignment, const FontDefinition &fontDef,
	const std::optional<TextRenderShadowInfo> &shadow, int lineSpacing)
{
	// Each offset points to the top-left corner of where the line should be rendered.
	Buffer<Int2> offsets(textLines.getCount());

	// X offsets.
	if ((alignment == TextAlignment::TopLeft) ||
		(alignment == TextAlignment::MiddleLeft) ||
		(alignment == TextAlignment::BottomLeft))
	{
		// Text lines are against the left edge.
		for (Int2 &offset : offsets)
		{
			offset.x = 0;
		}
	}
	else if ((alignment == TextAlignment::TopCenter) ||
		(alignment == TextAlignment::MiddleCenter) ||
		(alignment == TextAlignment::BottomCenter))
	{
		// Text lines are centered horizontally around the middle of the texture.
		for (int i = 0; i < textLines.getCount(); i++)
		{
			const std::string_view textLine = textLines[i];
			const int linePixelWidth = TextRenderUtils::getLinePixelWidth(textLine, fontDef, shadow);
			offsets[i].x = (textureWidth / 2) - (linePixelWidth / 2);
		}
	}
	else if ((alignment == TextAlignment::TopRight) ||
		(alignment == TextAlignment::MiddleRight) ||
		(alignment == TextAlignment::BottomRight))
	{
		// Text lines are against the right edge.
		for (int i = 0; i < textLines.getCount(); i++)
		{
			const std::string_view textLine = textLines[i];
			const int linePixelWidth = TextRenderUtils::getLinePixelWidth(textLine, fontDef, shadow);
			offsets[i].x = textureWidth - linePixelWidth;
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(alignment)));
	}

	// Y offsets.
	if ((alignment == TextAlignment::TopLeft) ||
		(alignment == TextAlignment::TopCenter) ||
		(alignment == TextAlignment::TopRight))
	{
		// The top text line is against the top of the texture.
		for (int i = 0; i < textLines.getCount(); i++)
		{
			offsets[i].y = (fontDef.characterHeight + lineSpacing) * i;
		}
	}
	else if ((alignment == TextAlignment::MiddleLeft) ||
		(alignment == TextAlignment::MiddleCenter) ||
		(alignment == TextAlignment::MiddleRight))
	{
		// Text lines are centered vertically around the middle of the texture.
		const int totalTextHeight = TextRenderUtils::getLinesPixelHeight(textLines, fontDef, shadow, lineSpacing);

		for (int i = 0; i < textLines.getCount(); i++)
		{
			offsets[i].y = ((textureHeight / 2) - (totalTextHeight / 2)) + ((fontDef.characterHeight + lineSpacing) * i);
		}
	}
	else if ((alignment == TextAlignment::BottomLeft) ||
		(alignment == TextAlignment::BottomCenter) ||
		(alignment == TextAlignment::BottomRight))
	{
		// The bottom text line is against the bottom of the texture.
		for (int i = 0; i < textLines.getCount(); i++)
		{
			offsets[i].y = textureHeight - fontDef.characterHeight - ((fontDef.characterHeight + lineSpacing) * (textLines.getCount() - 1 - i));
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(alignment)));
	}

	return offsets;
}

void TextRenderUtils::drawChar(const FontDefinitionCharacter &fontChar, int dstX, int dstY, Color textColor, Span2D<uint32_t> outBuffer)
{
	// @todo: clip loop ranges instead of checking in loop.
	for (int y = dstY; y < (dstY + fontChar.getHeight()); y++)
	{
		for (int x = dstX; x < (dstX + fontChar.getWidth()); x++)
		{
			if ((x >= 0) && (x < outBuffer.getWidth()) && (y >= 0) && (y < outBuffer.getHeight()))
			{
				const int srcX = x - dstX;
				const int srcY = y - dstY;
				const bool srcPixelIsColored = fontChar.get(srcX, srcY);
				if (srcPixelIsColored)
				{
					const uint32_t dstPixel = textColor.toRGBA();
					outBuffer.set(x, y, dstPixel);
				}
			}
		}
	}
}

void TextRenderUtils::drawTextLine(Span<const FontDefinitionCharacterID> charIDs, const FontDefinition &fontDef,
	int dstX, int dstY, Color textColor, Span<const TextRenderTabColorOverrideEntry> tabColorOverrideEntries,
	const TextRenderShadowInfo *shadow, Span2D<uint32_t> outBuffer)
{
	auto drawLine = [&charIDs, &fontDef, &tabColorOverrideEntries, &outBuffer](int x, int y, Color color, bool allowTabColors)
	{
		int currentX = 0;
		Color currentColor = color;

		for (int i = 0; i < charIDs.getCount(); i++)
		{
			const FontDefinitionCharacterID charID = charIDs[i];
			if (charID < 0)
			{
				// Non-presentable character (like a tab '\t').
				continue;
			}

			if (allowTabColors)
			{
				const auto tabColorIter = std::find_if(tabColorOverrideEntries.begin(), tabColorOverrideEntries.end(),
					[i](const TextRenderTabColorOverrideEntry &entry)
				{
					return entry.charIdIndex == i;
				});

				if (tabColorIter != tabColorOverrideEntries.end())
				{
					currentColor = tabColorIter->color;
				}
			}

			const FontDefinitionCharacter &fontChar = fontDef.getCharacter(charID);
			TextRenderUtils::drawChar(fontChar, x + currentX, y, currentColor, outBuffer);
			currentX += fontChar.getWidth();
		}
	};

	int foregroundDstX = dstX;
	int foregroundDstY = dstY;
	if (shadow != nullptr)
	{
		foregroundDstX += std::max(-shadow->offsetX, 0);
		foregroundDstY += std::max(-shadow->offsetY, 0);

		const int shadowDstX = dstX + std::max(shadow->offsetX, 0);
		const int shadowDstY = dstY + std::max(shadow->offsetY, 0);
		drawLine(shadowDstX, shadowDstY, shadow->color, false);
	}

	drawLine(foregroundDstX, foregroundDstY, textColor, true);
}

void TextRenderUtils::drawTextLine(const std::string_view line, const FontDefinition &fontDef, int dstX, int dstY,
	Color textColor, const Palette *tabColorPalette, const TextRenderShadowInfo *shadow, Span2D<uint32_t> outBuffer)
{
	const Buffer<FontDefinitionCharacterID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
	const std::vector<TextRenderTabColorOverrideEntry> tabColorOverrideEntries = TextRenderUtils::getLineTabColorOverrideEntries(line, tabColorPalette);
	TextRenderUtils::drawTextLine(charIDs, fontDef, dstX, dstY, textColor, tabColorOverrideEntries, shadow, outBuffer);
}

void TextRenderUtils::drawTextLines(Span<const std::string_view> textLines, const FontDefinition &fontDef, int dstX, int dstY,
	Color textColor, const Palette *tabColorPalette, TextAlignment alignment, int lineSpacing, const TextRenderShadowInfo *shadow, Span2D<uint32_t> outBuffer)
{
	// @todo: should pass std::optional parameter instead
	std::optional<TextRenderShadowInfo> shadowInfo;
	if (shadow != nullptr)
	{
		shadowInfo = *shadow;
	}

	const int textureWidth = outBuffer.getWidth();
	const int textureHeight = outBuffer.getHeight();
	const Buffer<Int2> offsets = TextRenderUtils::makeAlignmentOffsets(textLines, textureWidth, textureHeight, alignment, fontDef, shadowInfo, lineSpacing);
	DebugAssert(offsets.getCount() == textLines.getCount());

	// Draw text to texture.
	// @todo: might need to draw all shadow lines before all regular lines.
	for (int i = 0; i < textLines.getCount(); i++)
	{
		const std::string_view textLine = textLines[i];
		const Int2 offset = offsets[i];
		TextRenderUtils::drawTextLine(textLine, fontDef, dstX + offset.x, dstY + offset.y, textColor, tabColorPalette, shadow, outBuffer);
	}
}
