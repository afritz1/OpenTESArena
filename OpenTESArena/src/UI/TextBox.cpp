#include <algorithm>

#include "SDL.h"

#include "FontDefinition.h"
#include "FontLibrary.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

TextBoxProperties::TextBoxProperties(int fontDefIndex, const TextRenderTextureGenInfo &textureGenInfo, const Color &defaultColor,
	TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadowInfo, int lineSpacing)
	: textureGenInfo(textureGenInfo), defaultColor(defaultColor), shadowInfo(shadowInfo)
{
	this->fontDefIndex = fontDefIndex;
	this->alignment = alignment;
	this->lineSpacing = lineSpacing;
}

TextBoxProperties::TextBoxProperties()
	: TextBoxProperties(-1, TextRenderTextureGenInfo(), Color(), static_cast<TextAlignment>(-1), std::nullopt, 0) { }

void TextBoxInitInfo::init(const Rect &rect, TextBoxProperties &&properties)
{
	this->rect = rect;
	this->properties = std::move(properties);
}

TextBoxInitInfo TextBoxInitInfo::makeWithCenter(const std::string_view text, const Int2 &center,
	const std::string &fontName, const Color &textColor, TextAlignment alignment,
	const std::optional<TextRenderShadowInfo> &shadow, int lineSpacing, const FontLibrary &fontLibrary)
{
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo =
		TextRenderUtils::makeTextureGenInfo(text, fontDef, shadow, lineSpacing);

	const Rect rect(center, textureGenInfo.width, textureGenInfo.height);
	TextBoxProperties properties(fontDefIndex, textureGenInfo, textColor, alignment, shadow, lineSpacing);

	TextBoxInitInfo initInfo;
	initInfo.init(rect, std::move(properties));
	return initInfo;
}

TextBoxInitInfo TextBoxInitInfo::makeWithCenter(const std::string_view text, const Int2 &center,
	const std::string &fontName, const Color &textColor, TextAlignment alignment, const FontLibrary &fontLibrary)
{
	const std::optional<TextRenderShadowInfo> shadow;
	constexpr int lineSpacing = 0;
	return TextBoxInitInfo::makeWithCenter(text, center, fontName, textColor, alignment, shadow, lineSpacing, fontLibrary);
}

TextBoxInitInfo TextBoxInitInfo::makeWithXY(const std::string_view text, int x, int y, const std::string &fontName,
	const Color &textColor, TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadow,
	int lineSpacing, const FontLibrary &fontLibrary)
{
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo =
		TextRenderUtils::makeTextureGenInfo(text, fontDef, shadow, lineSpacing);

	const Rect rect(x, y, textureGenInfo.width, textureGenInfo.height);
	TextBoxProperties properties(fontDefIndex, textureGenInfo, textColor, alignment, shadow, lineSpacing);

	TextBoxInitInfo initInfo;
	initInfo.init(rect, std::move(properties));
	return initInfo;
}

TextBoxInitInfo TextBoxInitInfo::makeWithXY(const std::string_view text, int x, int y,
	const std::string &fontName, const Color &textColor, TextAlignment alignment, const FontLibrary &fontLibrary)
{
	const std::optional<TextRenderShadowInfo> shadow;
	constexpr int lineSpacing = 0;
	return TextBoxInitInfo::makeWithXY(text, x, y, fontName, textColor, alignment, shadow, lineSpacing, fontLibrary);
}

TextBox::TextBox()
{
	this->dirty = false;
}

bool TextBox::init(const Rect &rect, const TextBoxProperties &properties, Renderer &renderer)
{
	this->rect = rect;
	this->properties = properties;

	const int textureWidth = properties.textureGenInfo.width;
	const int textureHeight = properties.textureGenInfo.height;
	const UiTextureID textureID = renderer.createUiTexture(textureWidth, textureHeight);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create UI texture for text box with dims %dx%d.", textureWidth, textureHeight);
		return false;
	}
	
	this->textureRef.init(textureID, renderer);
	this->dirty = true;
	return true;
}

bool TextBox::init(const TextBoxInitInfo &initInfo, Renderer &renderer)
{
	return this->init(initInfo.rect, initInfo.properties, renderer);
}

bool TextBox::init(const TextBoxInitInfo &initInfo, const std::string_view text, Renderer &renderer)
{
	if (!this->init(initInfo.rect, initInfo.properties, renderer))
	{
		return false;
	}

	this->setText(text);
	return true;
}

const Rect &TextBox::getRect() const
{
	return this->rect;
}

UiTextureID TextBox::getTextureID()
{
	if (this->dirty)
	{
		this->updateTexture();

		if (this->dirty)
		{
			std::string textPreview;
			constexpr int maxTextPreviewLength = 15;
			if (this->text.size() < maxTextPreviewLength)
			{
				textPreview = this->text;
			}
			else
			{
				textPreview = this->text.substr(0, maxTextPreviewLength) + "...";
			}

			DebugLogError("Text box \"" + textPreview + "\" did not update its UI texture properly.");
		}
	}

	return this->textureRef.get();
}

void TextBox::setText(const std::string_view text)
{
	this->text = std::string(text);
	this->dirty = true;
}

void TextBox::addOverrideColor(int charIndex, const Color &overrideColor)
{
	this->colorOverrideInfo.add(charIndex, overrideColor);
	this->dirty = true;
}

void TextBox::clearOverrideColors()
{
	this->colorOverrideInfo.clear();
	this->dirty = true;
}

void TextBox::updateTexture()
{
	if (!this->dirty)
	{
		return;
	}

	LockedTexture lockedTexture = this->textureRef.lockTexels();
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock text box UI texture for updating.");
		return;
	}

	Span2D<uint32_t> texels = lockedTexture.getTexels32();
	texels.fill(0);

	if (!this->text.empty())
	{
		const FontLibrary &fontLibrary = FontLibrary::getInstance();
		const FontDefinition &fontDef = fontLibrary.getDefinition(this->properties.fontDefIndex);

		const Buffer<std::string_view> textLines = TextRenderUtils::getTextLines(this->text);
		const TextRenderColorOverrideInfo *colorOverrideInfoPtr = (this->colorOverrideInfo.getEntryCount() > 0) ? &this->colorOverrideInfo : nullptr;
		const TextRenderShadowInfo *shadowInfoPtr = this->properties.shadowInfo.has_value() ? &(*this->properties.shadowInfo) : nullptr;
		TextRenderUtils::drawTextLines(textLines, fontDef, 0, 0, this->properties.defaultColor, this->properties.alignment,
			this->properties.lineSpacing, colorOverrideInfoPtr, shadowInfoPtr, texels);
	}

	this->textureRef.unlockTexels();
	this->dirty = false;
}
