#include <algorithm>

#include "UiMessageBox.h"
#include "../Rendering/Renderer.h"

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

UiMessageBoxInitInfo::UiMessageBoxInitInfo()
{
	this->titleMinWidth = 0;
	this->titleMinHeight = 0;
	this->titleLineSpacing = 0;

	std::fill(std::begin(this->buttonCallbacks), std::end(this->buttonCallbacks), [](MouseButtonType) { });
	this->buttonMinHeight = 0;
	this->buttonCount = 0;

	this->patternType = static_cast<UiTexturePatternType>(-1);
	this->paddingWidth = 0;
	this->paddingHeight = 0;
}

void UiMessageBoxInitInfo::addButton(const std::string &text, const UiMessageBoxButtonCallback &callback)
{
	if (this->buttonCount >= UI_MESSAGE_BOX_MAX_BUTTONS)
	{
		DebugLogErrorFormat("Can't add any more message box buttons (text: %s).", text.c_str());
		return;
	}

	this->buttonTexts[this->buttonCount] = text;
	this->buttonCallbacks[this->buttonCount] = callback;
	this->buttonCount++;
}

UiMessageBox::UiMessageBox()
{
	this->textureID = -1;
	this->paletteID = -1;

	this->titleFontDefIndex = -1;
	this->titleMinWidth = 0;
	this->titleMinHeight = 0;
	this->titleLineSpacing = 0;

	std::fill(std::begin(this->buttonCallbacks), std::end(this->buttonCallbacks), [](MouseButtonType) { });
	this->buttonFontDefIndex = -1;
	this->buttonMinHeight = 0;
	this->buttonCount = 0;

	this->patternType = static_cast<UiTexturePatternType>(-1);
	this->paddingWidth = 0;
	this->paddingHeight = 0;

	this->dirty = false;
}

void UiMessageBox::init(UiTextureID textureID, PaletteID paletteID, const std::string &titleText, int titleFontDefIndex, Color titleTextColor,
	int titleMinWidth, int titleMinHeight, int titleLineSpacing, Span<const std::string> buttonTexts,
	Span<const UiMessageBoxButtonCallback> buttonCallbacks, int buttonFontDefIndex, Color buttonTextColor, int buttonMinHeight,
	int buttonCount, UiTexturePatternType patternType, int paddingWidth, int paddingHeight)
{
	this->textureID = textureID;
	this->paletteID = paletteID;

	this->titleText = titleText;
	this->titleFontDefIndex = titleFontDefIndex;
	this->titleTextColor = titleTextColor;
	this->titleMinWidth = titleMinWidth;
	this->titleMinHeight = titleMinHeight;
	this->titleLineSpacing = titleLineSpacing;

	DebugAssert(buttonTexts.getCount() <= UI_MESSAGE_BOX_MAX_BUTTONS);
	DebugAssert(buttonCallbacks.getCount() <= UI_MESSAGE_BOX_MAX_BUTTONS);
	std::copy(buttonTexts.begin(), buttonTexts.end(), std::begin(this->buttonTexts));
	std::copy(buttonCallbacks.begin(), buttonCallbacks.end(), std::begin(this->buttonCallbacks));

	this->buttonFontDefIndex = buttonFontDefIndex;
	this->buttonTextColor = buttonTextColor;
	this->buttonMinHeight = buttonMinHeight;
	this->buttonCount = buttonCount;

	this->patternType = patternType;
	this->paddingWidth = paddingWidth;
	this->paddingHeight = paddingHeight;

	this->dirty = true;
}

void UiMessageBox::free(Renderer &renderer)
{
	if (this->textureID >= 0)
	{
		renderer.freeUiTexture(this->textureID);
		this->textureID = -1;
	}
}
