#include <algorithm>

#include "FontLibrary.h"
#include "UiListBox.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiListBoxInitInfo::UiListBoxInitInfo()
{
	this->textureWidth = 0;
	this->textureHeight = 0;
	this->columnPixelXOffsets = { 0 };
	this->itemPixelSpacing = 0;
	this->mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->scrollDeltaScale = 1.0;
}

UiListBoxItem::UiListBoxItem()
{
	this->callback = [](MouseButtonType) { };
}

void UiListBoxItem::init(Span<const std::string> textColumns, const std::optional<Color> &overrideColor, const UiListBoxItemCallback &callback)
{
	this->textColumns.resize(textColumns.getCount());
	std::copy(textColumns.begin(), textColumns.end(), this->textColumns.begin());

	this->overrideColor = overrideColor;
	this->callback = callback;
}

void UiListBoxItem::init(const std::string &text, const std::optional<Color> &overrideColor, const UiListBoxItemCallback &callback)
{
	const Span<const std::string> textView(&text, 1);
	this->init(textView, overrideColor, callback);
}

UiListBox::UiListBox()
{
	this->textureID = -1;
	this->textureWidth = 0;
	this->textureHeight = 0;
	this->columnPixelXOffsets = { 0 };
	this->itemPixelSpacing = 0;
	this->fontDefIndex = -1;
	this->mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->scrollDeltaScale = 1.0;
	this->scrollPixelOffset = 0.0;
	this->highlightedItemIndex = 0;
	this->dirty = false;
}

void UiListBox::init(UiTextureID textureID, int textureWidth, int textureHeight, Span<const int> columnPixelXOffsets, int itemPixelSpacing,
	int fontDefIndex, Color defaultTextColor, std::optional<Color> highlightedTextColor, MouseButtonTypeFlags mouseButtonTypeFlags, double scrollDeltaScale)
{
	DebugAssert(textureID >= 0);
	DebugAssert(textureWidth > 0);
	DebugAssert(textureHeight > 0);
	DebugAssert(itemPixelSpacing >= 0);
	DebugAssert(scrollDeltaScale > 0.0);
	this->textureID = textureID;
	this->textureWidth = textureWidth;
	this->textureHeight = textureHeight;
	
	this->columnPixelXOffsets.resize(columnPixelXOffsets.getCount());
	std::copy(columnPixelXOffsets.begin(), columnPixelXOffsets.end(), this->columnPixelXOffsets.begin());

	this->itemPixelSpacing = itemPixelSpacing;
	this->fontDefIndex = fontDefIndex;
	this->defaultTextColor = defaultTextColor;
	this->highlightedTextColor = highlightedTextColor;
	this->mouseButtonFlags = mouseButtonTypeFlags;
	this->scrollDeltaScale = scrollDeltaScale;
	this->scrollPixelOffset = 0.0;
	this->highlightedItemIndex = 0;
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
	const int itemHeight = fontDef.characterHeight;
	const double baseYOffset = static_cast<double>((index * (itemHeight + this->itemPixelSpacing)));
	return baseYOffset - this->scrollPixelOffset;
}

double UiListBox::getScrollDeltaPixels() const
{
	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const FontDefinition &fontDef = fontLibrary.getDefinition(this->fontDefIndex);
	const int itemHeight = fontDef.characterHeight;
	return static_cast<double>(itemHeight + this->itemPixelSpacing) * this->scrollDeltaScale;
}
