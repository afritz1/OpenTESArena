#include "FontLibrary.h"
#include "UiListBox.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiListBoxInitInfo::UiListBoxInitInfo()
{
	this->textureWidth = 0;
	this->textureHeight = 0;
	this->itemPixelSpacing = 0;
	this->scrollDeltaScale = 1.0;
}

UiListBoxItem::UiListBoxItem()
{
	this->callback = []() { };
}

UiListBox::UiListBox()
{
	this->textureID = -1;
	this->textureWidth = 0;
	this->textureHeight = 0;
	this->itemPixelSpacing = 0;
	this->fontDefIndex = -1;
	this->scrollDeltaScale = 1.0;
	this->scrollPixelOffset = 0.0;
	this->dirty = false;
}

void UiListBox::init(UiTextureID textureID, int textureWidth, int textureHeight, int itemPixelSpacing, int fontDefIndex, Color defaultTextColor, double scrollDeltaScale)
{
	DebugAssert(textureID >= 0);
	DebugAssert(textureWidth > 0);
	DebugAssert(textureHeight > 0);
	DebugAssert(itemPixelSpacing >= 0);
	DebugAssert(scrollDeltaScale > 0.0);
	this->textureID = textureID;
	this->textureWidth = textureWidth;
	this->textureHeight = textureHeight;
	this->itemPixelSpacing = itemPixelSpacing;
	this->fontDefIndex = fontDefIndex;
	this->defaultTextColor = defaultTextColor;
	this->scrollDeltaScale = scrollDeltaScale;
	this->scrollPixelOffset = 0.0;
	this->items.clear();
	this->dirty = true;
}

void UiListBox::free(Renderer &renderer)
{
	if (this->textureID >= 0)
	{
		renderer.freeUiTexture(this->textureID);
		this->textureID = -1;
	}
}

double UiListBox::getItemCurrentLocalY(int index) const
{
	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const FontDefinition &fontDef = fontLibrary.getDefinition(this->fontDefIndex);
	const int itemHeight = fontDef.getCharacterHeight();
	const double baseYOffset = static_cast<double>((index * (itemHeight + this->itemPixelSpacing)));
	return baseYOffset - this->scrollPixelOffset;
}

double UiListBox::getScrollDeltaPixels() const
{
	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const FontDefinition &fontDef = fontLibrary.getDefinition(this->fontDefIndex);
	const int itemHeight = fontDef.getCharacterHeight();
	return static_cast<double>(itemHeight + this->itemPixelSpacing) * this->scrollDeltaScale;
}
