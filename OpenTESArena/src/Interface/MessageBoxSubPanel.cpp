#include "MessageBoxSubPanel.h"

MessageBoxBackgroundProperties::MessageBoxBackgroundProperties(UiTexturePatternType patternType,
	int extraTitleWidth, int extraTitleHeight, const std::optional<int> &widthOverride,
	const std::optional<int> &heightOverride, int itemTextureHeight)
	: widthOverride(widthOverride), heightOverride(heightOverride)
{
	this->patternType = patternType;
	this->extraTitleWidth = extraTitleWidth;
	this->extraTitleHeight = extraTitleHeight;
	this->itemTextureHeight = itemTextureHeight;
}

MessageBoxTitleProperties::MessageBoxTitleProperties(const std::string &fontName,
	const TextRenderTextureGenInfo &textureGenInfo, const Color &textColor, int lineSpacing)
	: fontName(fontName), textureGenInfo(textureGenInfo), textColor(textColor)
{
	this->lineSpacing = lineSpacing;
}

MessageBoxItemsProperties::MessageBoxItemsProperties(int count, const std::string &fontName,
	const TextRenderTextureGenInfo &textureGenInfo, const Color &textColor)
	: fontName(fontName), textureGenInfo(textureGenInfo), textColor(textColor)
{
	this->count = count;
}
