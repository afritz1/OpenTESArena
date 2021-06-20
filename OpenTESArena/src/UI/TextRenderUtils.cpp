#include <algorithm>
#include <cmath>

#include "TextAlignment.h"
#include "TextRenderUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

TextRenderUtils::TextureGenInfo::TextureGenInfo()
{
	this->width = 0;
	this->height = 0;
}

void TextRenderUtils::TextureGenInfo::init(int width, int height)
{
	this->width = width;
	this->height = height;
}

TextRenderUtils::ColorOverrideInfo::Entry::Entry(int charIndex, const Color &color)
	: color(color)
{
	this->charIndex = charIndex;
}

int TextRenderUtils::ColorOverrideInfo::getEntryCount() const
{
	return static_cast<int>(this->entries.size());
}

std::optional<int> TextRenderUtils::ColorOverrideInfo::findEntryIndex(int charIndex) const
{
	const auto iter = std::find_if(this->entries.begin(), this->entries.end(),
		[charIndex](const ColorOverrideInfo::Entry &entry)
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

const Color &TextRenderUtils::ColorOverrideInfo::getColor(int entryIndex) const
{
	DebugAssertIndex(this->entries, entryIndex);
	return this->entries[entryIndex].color;
}

void TextRenderUtils::ColorOverrideInfo::add(int charIndex, const Color &color)
{
	const std::optional<int> existingEntryIndex = this->findEntryIndex(charIndex);
	if (existingEntryIndex.has_value())
	{
		DebugLogError("Already have color override for char index \"" + std::to_string(charIndex) + "\".");
		return;
	}

	this->entries.emplace_back(Entry(charIndex, color));
}

void TextRenderUtils::ColorOverrideInfo::clear()
{
	this->entries.clear();
}

TextRenderUtils::TextShadowInfo::TextShadowInfo(int offsetX, int offsetY, const Color &color)
	: color(color)
{
	this->offsetX = offsetX;
	this->offsetY = offsetY;
}

TextRenderUtils::TextShadowInfo::TextShadowInfo()
{
	this->offsetX = 0;
	this->offsetY = 0;
}

void TextRenderUtils::TextShadowInfo::init(int offsetX, int offsetY, const Color &color)
{
	this->offsetX = offsetX;
	this->offsetY = offsetY;
	this->color = color;
}

std::vector<std::string_view> TextRenderUtils::getTextLines(const std::string_view &text)
{
	// @todo: might eventually handle "\r\n".
	return StringView::split(text, '\n');
}

std::vector<FontDefinition::CharID> TextRenderUtils::getLineFontCharIDs(const std::string_view &line,
	const FontDefinition &fontDef)
{
	FontDefinition::CharID fallbackCharID;
	if (!fontDef.tryGetCharacterID("?", &fallbackCharID))
	{
		DebugCrash("Couldn't get fallback font character ID from font \"" + fontDef.getName() + "\".");
	}

	// @todo: support more than ASCII
	std::vector<FontDefinition::CharID> charIDs;
	for (const char c : line)
	{
		const std::string charUtf8(1, c);
		FontDefinition::CharID charID;
		if (!fontDef.tryGetCharacterID(charUtf8.c_str(), &charID))
		{
			DebugLogWarning("Couldn't get font character ID for \"" + charUtf8 + "\".");
			charID = fallbackCharID;
		}

		charIDs.push_back(charID);
	}

	return charIDs;
}

int TextRenderUtils::getLinePixelLength(const std::vector<FontDefinition::CharID> &charIDs,
	const FontDefinition &fontDef)
{
	int width = 0;
	for (const FontDefinition::CharID charID : charIDs)
	{
		const FontDefinition::Character &fontChar = fontDef.getCharacter(charID);
		width += fontChar.getWidth();
	}

	return width;
}

TextRenderUtils::TextureGenInfo TextRenderUtils::makeTextureGenInfo(const std::string_view &text,
	const FontDefinition &fontDef, const TextShadowInfo *shadow, int lineSpacing)
{
	// Get the width of the longest line of text in pixels.
	int width = 0;
	const std::vector<std::string_view> textLines = TextRenderUtils::getTextLines(text);
	for (const std::string_view &line : textLines)
	{
		const std::vector<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
		const int lineWidth = TextRenderUtils::getLinePixelLength(charIDs, fontDef);
		width = std::max(width, lineWidth);
	}

	const int lineCount = static_cast<int>(textLines.size());
	int height = (fontDef.getCharacterHeight() * lineCount) + (lineSpacing * std::max(0, lineCount - 1));

	if (shadow != nullptr)
	{
		width += std::abs(shadow->offsetX);
		height += std::abs(shadow->offsetY);
	}

	TextureGenInfo textureGenInfo;
	textureGenInfo.init(width, height);
	return textureGenInfo;
}

std::vector<int> TextRenderUtils::makeAlignmentXOffsets(const std::vector<std::string_view> &textLines,
	TextAlignment alignment, const FontDefinition &fontDef)
{
	std::vector<int> xOffsets(textLines.size());

	if (alignment == TextAlignment::Left)
	{
		// All text lines are against the left edge.
		std::fill(xOffsets.begin(), xOffsets.end(), 0);
	}
	else if (alignment == TextAlignment::Center)
	{
		auto getTextLinePixelLength = [&textLines, &fontDef](int textLinesIndex)
		{
			DebugAssertIndex(textLines, textLinesIndex);
			const std::string_view &textLine = textLines[textLinesIndex];
			const std::vector<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(textLine, fontDef);
			return TextRenderUtils::getLinePixelLength(charIDs, fontDef);
		};

		// Find longest line then make all other lines' offsets relative to that one.
		const int longestLineIndex = [&textLines]()
		{
			const auto iter = std::max_element(textLines.begin(), textLines.end(),
				[](const std::string_view &a, const std::string_view &b)
			{
				return a.size() < b.size();
			});

			if (iter == textLines.end())
			{
				DebugCrash("Couldn't find longest line for text alignment offsets.");
			}

			return static_cast<int>(std::distance(textLines.begin(), iter));
		}();

		const int longestLinePixelLength = getTextLinePixelLength(longestLineIndex);

		for (int i = 0; i < static_cast<int>(textLines.size()); i++)
		{
			if (i != longestLineIndex)
			{
				const int linePixelLength = getTextLinePixelLength(i);
				xOffsets[i] = (longestLinePixelLength / 2) - (linePixelLength / 2);
			}
			else
			{
				// Longest line has no offset.
				xOffsets[i] = 0;
			}
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(alignment)));
	}

	return xOffsets;
}

void TextRenderUtils::drawChar(const FontDefinition::Character &fontChar, int dstX, int dstY, const Color &textColor,
	BufferView2D<uint32_t> &outBuffer)
{
	const Color &transparentColor = Color::Transparent;

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
				const Color &dstColor = srcPixelIsColored ? textColor : transparentColor;
				const uint32_t dstPixel = dstColor.toARGB();
				outBuffer.set(x, y, dstPixel);
			}
		}
	}
}

void TextRenderUtils::drawTextLine(const BufferView<const FontDefinition::CharID> &charIDs, const FontDefinition &fontDef,
	int dstX, int dstY, const Color &textColor, const ColorOverrideInfo *colorOverrideInfo, const TextShadowInfo *shadow,
	BufferView2D<uint32_t> &outBuffer)
{
	auto drawLine = [&charIDs, &fontDef, colorOverrideInfo, &outBuffer](
		int x, int y, const Color &color, bool allowColorOverrides)
	{
		int currentX = 0;
		for (int i = 0; i < charIDs.getCount(); i++)
		{
			const FontDefinition::CharID charID = charIDs.get(i);
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

	if (shadow != nullptr)
	{
		constexpr bool allowShadowColorOverrides = false;
		drawLine(dstX + shadow->offsetX, dstY + shadow->offsetY, shadow->color, allowShadowColorOverrides);
	}

	const bool allowForegroundColorOverrides = colorOverrideInfo != nullptr;
	drawLine(dstX, dstY, textColor, allowForegroundColorOverrides);
}

void TextRenderUtils::drawTextLine(const std::string_view &line, const FontDefinition &fontDef, int dstX, int dstY,
	const Color &textColor, const ColorOverrideInfo *colorOverrideInfo, const TextShadowInfo *shadow,
	BufferView2D<uint32_t> &outBuffer)
{
	const std::vector<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
	const BufferView<const FontDefinition::CharID> charIdsView(charIDs.data(), static_cast<int>(charIDs.size()));
	TextRenderUtils::drawTextLine(charIdsView, fontDef, dstX, dstY, textColor, colorOverrideInfo, shadow, outBuffer);
}
