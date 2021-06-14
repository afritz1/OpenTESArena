#include <algorithm>

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

TextRenderUtils::TextureGenInfo TextRenderUtils::makeTextureGenInfo(const std::string_view &text,
	const FontDefinition &fontDef, int lineSpacing)
{
	// Get the width of the longest line of text in pixels.
	int width = 0;
	const std::vector<std::string_view> textLines = TextRenderUtils::getTextLines(text);
	for (const std::string_view &line : textLines)
	{
		const std::vector<FontDefinition::CharID> charIDs = TextRenderUtils::getLineFontCharIDs(line, fontDef);

		int tempWidth = 0;
		for (const FontDefinition::CharID charID : charIDs)
		{
			const FontDefinition::Character &fontChar = fontDef.getCharacter(charID);
			tempWidth += fontChar.getWidth();
		}

		width = std::max(width, tempWidth);
	}

	const int lineCount = static_cast<int>(textLines.size());
	const int height = (fontDef.getCharacterHeight() * lineCount) + (lineSpacing * std::max(0, lineCount - 1));

	TextureGenInfo textureGenInfo;
	textureGenInfo.init(width, height);
	return textureGenInfo;
}

TextRenderUtils::TextureGenInfo TextRenderUtils::makeTextureGenInfo(const std::string_view &text,
	const FontDefinition &fontDef)
{
	constexpr int lineSpacing = 0;
	return TextRenderUtils::makeTextureGenInfo(text, fontDef, lineSpacing);
}

void TextRenderUtils::drawChar(const FontDefinition::Character &fontChar, int dstX, int dstY, const Color &textColor,
	BufferView2D<uint32_t> &outBuffer)
{
	// Clip source texture to fit in destination texture.
	const int srcStartX = std::max(0, -dstX);
	const int srcEndX = std::clamp(outBuffer.getWidth() - dstX, 0, fontChar.getWidth());
	const int srcStartY = std::max(0, -dstY);
	const int srcEndY = std::clamp(outBuffer.getHeight() - dstY, 0, fontChar.getHeight());

	const Color &transparentColor = Color::Transparent;
	const int endX = srcEndX - srcStartX;
	const int endY = srcEndY - srcStartY;
	for (int y = 0; y < endY; y++)
	{
		for (int x = 0; x < endX; x++)
		{
			const bool srcPixelIsColored = fontChar.get(srcStartX + x, srcStartY + y);
			const Color &dstColor = srcPixelIsColored ? textColor : transparentColor;
			const uint32_t dstPixel = dstColor.toARGB();
			outBuffer.set(dstX + x, dstY + y, dstPixel);
		}
	}
}

void TextRenderUtils::drawTextLine(const BufferView<FontDefinition::CharID> &charIDs, const FontDefinition &fontDef,
	int dstX, int dstY, const Color &textColor, BufferView2D<uint32_t> &outBuffer)
{
	int currentX = 0;
	for (int i = 0; i < charIDs.getCount(); i++)
	{
		const FontDefinition::CharID charID = charIDs.get(i);
		const FontDefinition::Character &fontChar = fontDef.getCharacter(charID);
		TextRenderUtils::drawChar(fontChar, dstX + currentX, dstY, textColor, outBuffer);
		currentX += fontChar.getWidth();
	}
}
