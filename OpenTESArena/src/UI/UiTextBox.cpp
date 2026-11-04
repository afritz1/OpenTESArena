#include "TextAlignment.h"
#include "UiTextBox.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiTextBoxInitInfo::UiTextBoxInitInfo()
{
	this->defaultColor = Colors::White;
	this->tabColorPaletteID = -1;
	this->alignment = TextAlignment::TopLeft;
	this->lineSpacing = 0;
	this->hasCursor = false;
}

UiTextBox::UiTextBox()
{
	this->dirty = false;
	this->textureID = -1;
	this->textureWidth = -1;
	this->textureHeight = -1;
	this->fontDefIndex = -1;
	this->tabColorPaletteID = -1;
	this->alignment = TextAlignment::TopLeft;
	this->lineSpacing = -1;
	this->hasCursor = false;
	this->cursorCurrentSeconds = 0.0;
}

void UiTextBox::init(UiTextureID textureID, int textureWidth, int textureHeight, int fontDefIndex, const Color &defaultColor,
	PaletteID tabColorPaletteID, TextAlignment alignment, const std::optional<TextRenderShadowInfo> &shadowInfo, int lineSpacing,
	bool hasCursor)
{
	DebugAssert(textureID >= 0);
	DebugAssert(textureWidth > 0);
	DebugAssert(textureHeight > 0);
	DebugAssert(lineSpacing >= 0);
	this->text.clear();
	this->dirty = true;
	this->textureID = textureID;
	this->textureWidth = textureWidth;
	this->textureHeight = textureHeight;
	this->fontDefIndex = fontDefIndex;
	this->defaultColor = defaultColor;
	this->tabColorPaletteID = tabColorPaletteID;
	this->alignment = alignment;
	this->shadowInfo = shadowInfo;
	this->lineSpacing = lineSpacing;
	this->hasCursor = hasCursor;
	this->cursorCurrentSeconds = 0.0;
}

void UiTextBox::free(Renderer &renderer)
{
	if (this->textureID >= 0)
	{
		renderer.freeUiTexture(this->textureID);
		this->textureID = -1;
	}
}
