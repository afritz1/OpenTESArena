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

int TextRenderUtils::getLinePixelWidth(const std::vector<FontDefinition::CharID> &charIDs,
	const FontDefinition &fontDef, const std::optional<TextShadowInfo> &shadow)
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

int TextRenderUtils::getLinePixelWidth(const std::string_view &line, const FontDefinition &fontDef,
	const std::optional<TextShadowInfo> &shadow)
{
	const std::vector<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
	return TextRenderUtils::getLinePixelWidth(charIDs, fontDef, shadow);
}

int TextRenderUtils::getLinesPixelWidth(const BufferView<const std::string_view> &textLines, const FontDefinition &fontDef,
	const std::optional<TextShadowInfo> &shadow)
{
	int width = 0;
	for (int i = 0; i < textLines.getCount(); i++)
	{
		const std::string_view &line = textLines.get(i);
		const int linePixelWidth = TextRenderUtils::getLinePixelWidth(line, fontDef, shadow);
		width = std::max(width, linePixelWidth);
	}

	return width;
}

int TextRenderUtils::getLinesPixelHeight(const BufferView<const std::string_view> &textLines, const FontDefinition &fontDef,
	const std::optional<TextShadowInfo> &shadow, int lineSpacing)
{
	const int lineCount = textLines.getCount();
	return (fontDef.getCharacterHeight() * lineCount) + (lineSpacing * std::max(0, lineCount - 1)) +
		(shadow.has_value() ? std::abs(shadow->offsetY) : 0);
}

TextRenderUtils::TextureGenInfo TextRenderUtils::makeTextureGenInfo(const BufferView<const std::string_view> &textLines,
	const FontDefinition &fontDef, const std::optional<TextShadowInfo> &shadow, int lineSpacing)
{
	const int width = TextRenderUtils::getLinesPixelWidth(textLines, fontDef, shadow);
	const int height = TextRenderUtils::getLinesPixelHeight(textLines, fontDef, shadow, lineSpacing);

	TextureGenInfo textureGenInfo;
	textureGenInfo.init(width, height);
	return textureGenInfo;
}

TextRenderUtils::TextureGenInfo TextRenderUtils::makeTextureGenInfo(const std::string_view &text,
	const FontDefinition &fontDef, const std::optional<TextShadowInfo> &shadow, int lineSpacing)
{
	const std::vector<std::string_view> textLines = TextRenderUtils::getTextLines(text);
	const BufferView<const std::string_view> textLinesView(textLines.data(), static_cast<int>(textLines.size()));
	return TextRenderUtils::makeTextureGenInfo(textLinesView, fontDef, shadow, lineSpacing);
}

std::vector<Int2> TextRenderUtils::makeAlignmentOffsets(const BufferView<const std::string_view> &textLines,
	int textureWidth, int textureHeight, TextAlignment alignment, const FontDefinition &fontDef,
	const std::optional<TextShadowInfo> &shadow, int lineSpacing)
{
	// Each offset points to the top-left corner of where the line should be rendered.
	std::vector<Int2> offsets(textLines.getCount());

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
			const std::string_view &textLine = textLines.get(i);
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
			const std::string_view &textLine = textLines.get(i);
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
	BufferView2D<uint32_t> &outBuffer)
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
					const uint32_t dstPixel = textColor.toARGB();
					outBuffer.set(x, y, dstPixel);
				}
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

void TextRenderUtils::drawTextLine(const std::string_view &line, const FontDefinition &fontDef, int dstX, int dstY,
	const Color &textColor, const ColorOverrideInfo *colorOverrideInfo, const TextShadowInfo *shadow,
	BufferView2D<uint32_t> &outBuffer)
{
	const std::vector<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);
	const BufferView<const FontDefinition::CharID> charIdsView(charIDs.data(), static_cast<int>(charIDs.size()));
	TextRenderUtils::drawTextLine(charIdsView, fontDef, dstX, dstY, textColor, colorOverrideInfo, shadow, outBuffer);
}

void TextRenderUtils::drawTextLines(const BufferView<const std::string_view> &textLines, const FontDefinition &fontDef,
	int dstX, int dstY, const Color &textColor, TextAlignment alignment, int lineSpacing,
	const ColorOverrideInfo *colorOverrideInfo, const TextShadowInfo *shadow, BufferView2D<uint32_t> &outBuffer)
{
	// @todo: should pass std::optional parameter instead
	std::optional<TextRenderUtils::TextShadowInfo> shadowInfo;
	if (shadow != nullptr)
	{
		shadowInfo = *shadow;
	}

	const int textureWidth = outBuffer.getWidth();
	const int textureHeight = outBuffer.getHeight();
	const std::vector<Int2> offsets = TextRenderUtils::makeAlignmentOffsets(
		textLines, textureWidth, textureHeight, alignment, fontDef, shadowInfo, lineSpacing);
	DebugAssert(offsets.size() == textLines.getCount());

	// Draw text to texture.
	// @todo: might need to draw all shadow lines before all regular lines.
	for (int i = 0; i < textLines.getCount(); i++)
	{
		const std::string_view &textLine = textLines.get(i);
		const Int2 &offset = offsets[i];
		TextRenderUtils::drawTextLine(textLine, fontDef, dstX + offset.x, dstY + offset.y, textColor,
			colorOverrideInfo, shadow, outBuffer);
	}
}
