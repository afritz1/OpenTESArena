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

TextRenderColorOverrideInfoEntry::TextRenderColorOverrideInfoEntry(int charIndex, const Color &color)
	: color(color)
{
	this->charIndex = charIndex;
}

std::vector<TextRenderColorOverrideInfoEntry> TextRenderColorOverrideInfo::makeEntriesFromText(
	const std::string_view text, const Palette &palette)
{
	// Technically the original game treats these as global color mode changes, not single-character overrides,
	// so that could be something better-handled maybe.
	std::vector<TextRenderColorOverrideInfoEntry> entries;

	for (size_t i = 0; i < text.size(); i++)
	{
		if ((text[i] == '\t') && (i < (text.size() - 2)))
		{
			const uint8_t paletteIndex = static_cast<uint8_t>(text[i + 1]);
			const Color &paletteColor = palette[paletteIndex];
			entries.emplace_back(TextRenderColorOverrideInfoEntry(static_cast<int>(i), paletteColor));
		}
	}

	return entries;
}

int TextRenderColorOverrideInfo::getEntryCount() const
{
	return static_cast<int>(this->entries.size());
}

std::optional<int> TextRenderColorOverrideInfo::findEntryIndex(int charIndex) const
{
	const auto iter = std::find_if(this->entries.begin(), this->entries.end(),
		[charIndex](const TextRenderColorOverrideInfoEntry &entry)
	{
		return entry.charIndex == charIndex;
	});

	if (iter != this->entries.end())
	{
		return static_cast<int>(std::distance(this->entries.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

const Color &TextRenderColorOverrideInfo::getColor(int entryIndex) const
{
	DebugAssertIndex(this->entries, entryIndex);
	return this->entries[entryIndex].color;
}

void TextRenderColorOverrideInfo::add(int charIndex, const Color &color)
{
	const std::optional<int> existingEntryIndex = this->findEntryIndex(charIndex);
	if (existingEntryIndex.has_value())
	{
		DebugLogError("Already have color override for char index \"" + std::to_string(charIndex) + "\".");
		return;
	}

	this->entries.emplace_back(TextRenderColorOverrideInfoEntry(charIndex, color));
}

void TextRenderColorOverrideInfo::clear()
{
	this->entries.clear();
}

TextRenderShadowInfo::TextRenderShadowInfo(int offsetX, int offsetY, const Color &color)
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

void TextRenderShadowInfo::init(int offsetX, int offsetY, const Color &color)
{
	this->offsetX = offsetX;
	this->offsetY = offsetY;
	this->color = color;
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

Buffer<FontDefinition::CharID> TextRenderUtils::getLineFontCharIDs(const std::string_view line, const FontDefinition &fontDef)
{
	FontDefinition::CharID fallbackCharID;
	if (!fontDef.tryGetCharacterID("?", &fallbackCharID))
	{
		DebugCrash("Couldn't get fallback font character ID from font \"" + fontDef.getName() + "\".");
	}

	const int lineLength = static_cast<int>(line.size());

	// @todo: support more than ASCII
	Buffer<FontDefinition::CharID> charIDs(lineLength);
	for (int i = 0; i < lineLength; i++)
	{
		const char c = line[i];
		const std::string charUtf8(1, c);
		FontDefinition::CharID charID;
		if (!fontDef.tryGetCharacterID(charUtf8.c_str(), &charID))
		{
			DebugLogWarning("Couldn't get font character ID for \"" + charUtf8 + "\".");
			charID = fallbackCharID;
		}

		charIDs[i] = charID;
	}

	return charIDs;
}

int TextRenderUtils::getLinePixelWidth(Span<const FontDefinition::CharID> charIDs,
	const FontDefinition &fontDef, const std::optional<TextRenderShadowInfo> &shadow)
{
	int width = 0;
	for (const FontDefinition::CharID charID : charIDs)
	{
		const FontDefinition::Character &fontChar = fontDef.getCharacter(charID);
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
	const Buffer<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
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
	return (fontDef.getCharacterHeight() * lineCount) + (lineSpacing * std::max(0, lineCount - 1)) +
		(shadow.has_value() ? std::abs(shadow->offsetY) : 0);
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
			offsets[i].y = (fontDef.getCharacterHeight() + lineSpacing) * i;
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
			offsets[i].y = ((textureHeight / 2) - (totalTextHeight / 2)) +
				((fontDef.getCharacterHeight() + lineSpacing) * i);
		}
	}
	else if ((alignment == TextAlignment::BottomLeft) ||
		(alignment == TextAlignment::BottomCenter) ||
		(alignment == TextAlignment::BottomRight))
	{
		// The bottom text line is against the bottom of the texture.
		for (int i = 0; i < textLines.getCount(); i++)
		{
			offsets[i].y = textureHeight - fontDef.getCharacterHeight() -
				((fontDef.getCharacterHeight() + lineSpacing) * (textLines.getCount() - 1 - i));
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(alignment)));
	}

	return offsets;
}

void TextRenderUtils::drawChar(const FontDefinition::Character &fontChar, int dstX, int dstY, const Color &textColor,
	Span2D<uint32_t> &outBuffer)
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

void TextRenderUtils::drawTextLine(Span<const FontDefinition::CharID> charIDs, const FontDefinition &fontDef,
	int dstX, int dstY, const Color &textColor, const TextRenderColorOverrideInfo *colorOverrideInfo, const TextRenderShadowInfo *shadow,
	Span2D<uint32_t> &outBuffer)
{
	auto drawLine = [&charIDs, &fontDef, colorOverrideInfo, &outBuffer](
		int x, int y, const Color &color, bool allowColorOverrides)
	{
		int currentX = 0;
		for (int i = 0; i < charIDs.getCount(); i++)
		{
			const FontDefinition::CharID charID = charIDs[i];
			const FontDefinition::Character &fontChar = fontDef.getCharacter(charID);
			const Color &charColor = [colorOverrideInfo, &color, allowColorOverrides, i]() -> const Color&
			{
				if (allowColorOverrides)
				{
					const std::optional<int> entryIndex = colorOverrideInfo->findEntryIndex(i);
					if (entryIndex.has_value())
					{
						return colorOverrideInfo->getColor(*entryIndex);
					}
				}

				return color;
			}();

			TextRenderUtils::drawChar(fontChar, x + currentX, y, charColor, outBuffer);
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
		constexpr bool allowShadowColorOverrides = false;
		drawLine(shadowDstX, shadowDstY, shadow->color, allowShadowColorOverrides);
	}

	const bool allowForegroundColorOverrides = colorOverrideInfo != nullptr;
	drawLine(foregroundDstX, foregroundDstY, textColor, allowForegroundColorOverrides);
}

void TextRenderUtils::drawTextLine(const std::string_view line, const FontDefinition &fontDef, int dstX, int dstY,
	const Color &textColor, const TextRenderColorOverrideInfo *colorOverrideInfo, const TextRenderShadowInfo *shadow,
	Span2D<uint32_t> &outBuffer)
{
	const Buffer<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
	const Span<const FontDefinition::CharID> charIdsView(charIDs);
	TextRenderUtils::drawTextLine(charIdsView, fontDef, dstX, dstY, textColor, colorOverrideInfo, shadow, outBuffer);
}

void TextRenderUtils::drawTextLines(Span<const std::string_view> textLines, const FontDefinition &fontDef,
	int dstX, int dstY, const Color &textColor, TextAlignment alignment, int lineSpacing,
	const TextRenderColorOverrideInfo *colorOverrideInfo, const TextRenderShadowInfo *shadow, Span2D<uint32_t> &outBuffer)
{
	// @todo: should pass std::optional parameter instead
	std::optional<TextRenderShadowInfo> shadowInfo;
	if (shadow != nullptr)
	{
		shadowInfo = *shadow;
	}

	const int textureWidth = outBuffer.getWidth();
	const int textureHeight = outBuffer.getHeight();
	const Buffer<Int2> offsets = TextRenderUtils::makeAlignmentOffsets(
		textLines, textureWidth, textureHeight, alignment, fontDef, shadowInfo, lineSpacing);
	DebugAssert(offsets.getCount() == textLines.getCount());

	// Draw text to texture.
	// @todo: might need to draw all shadow lines before all regular lines.
	for (int i = 0; i < textLines.getCount(); i++)
	{
		const std::string_view textLine = textLines[i];
		const Int2 &offset = offsets[i];
		TextRenderUtils::drawTextLine(textLine, fontDef, dstX + offset.x, dstY + offset.y, textColor,
			colorOverrideInfo, shadow, outBuffer);
	}
}
