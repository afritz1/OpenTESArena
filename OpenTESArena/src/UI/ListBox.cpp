#include <algorithm>

#include "SDL.h"

#include "FontDefinition.h"
#include "FontLibrary.h"
#include "ListBox.h"
#include "TextRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

ListBoxProperties::ListBoxProperties(int fontDefIndex, const TextRenderTextureGenInfo &textureGenInfo, int itemHeight,
	const Color &defaultColor, double scrollScale, int itemSpacing)
	: textureGenInfo(textureGenInfo), defaultColor(defaultColor)
{
	this->fontDefIndex = fontDefIndex;
	this->itemHeight = itemHeight;
	this->scrollScale = scrollScale;
	this->itemSpacing = itemSpacing;
}

ListBoxProperties::ListBoxProperties()
	: ListBoxProperties(-1, TextRenderTextureGenInfo(), 0, Color(), 0.0, 0) { }

void ListBoxItem::init(std::string &&text, const std::optional<Color> &overrideColor, const ListBoxItemCallback &callback)
{
	this->text = std::move(text);
	this->overrideColor = overrideColor;
	this->callback = callback;
}

ListBox::ListBox()
{
	this->scrollPixelOffset = 0.0;
	this->dirty = false;
}

bool ListBox::init(const Rect &rect, const ListBoxProperties &properties, Renderer &renderer)
{
	this->rect = rect;
	this->properties = properties;

	const int textureWidth = properties.textureGenInfo.width;
	const int textureHeight = properties.textureGenInfo.height;
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(textureWidth, textureHeight, &textureID))
	{
		DebugLogError("Couldn't create UI texture for list box (dims: " + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + ").");
		return false;
	}

	this->textureRef.init(textureID, renderer);
	this->dirty = true;
	return true;
}

const Rect &ListBox::getRect() const
{
	return this->rect;
}

Rect ListBox::getItemLocalRect(int index) const
{
	const double baseYOffset = static_cast<double>(
		(index * (this->properties.itemHeight + this->properties.itemSpacing)));
	return Rect(
		0,
		static_cast<int>(baseYOffset - this->scrollPixelOffset),
		this->rect.width,
		this->properties.itemHeight);
}

Rect ListBox::getItemGlobalRect(int index) const
{
	const Rect localRect = this->getItemLocalRect(index);
	return Rect(
		this->rect.getLeft() + localRect.getLeft(),
		this->rect.getTop() + localRect.getTop(),
		localRect.width,
		localRect.height);
}

int ListBox::getCount() const
{
	return static_cast<int>(this->items.size());
}

const ListBoxItemCallback &ListBox::getCallback(int index) const
{
	DebugAssertIndex(this->items, index);
	return this->items[index].callback;
}

int ListBox::getFirstVisibleItemIndex() const
{
	return static_cast<int>(this->scrollPixelOffset) / (this->properties.itemHeight + this->properties.itemSpacing);
}

UiTextureID ListBox::getTextureID()
{
	if (this->dirty)
	{
		this->updateTexture();
		DebugAssert(!this->dirty);
	}

	return this->textureRef.get();
}

double ListBox::getScrollDeltaPixels() const
{
	return static_cast<double>(this->properties.itemHeight + this->properties.itemSpacing) * this->properties.scrollScale;
}

void ListBox::insert(int index, std::string &&text)
{
	DebugAssert(index >= 0);
	DebugAssert(index <= static_cast<int>(this->items.size())); // One past end is okay.

	ListBoxItem item;
	item.init(std::move(text), std::nullopt, []() { });
	this->items.insert(this->items.begin() + index, std::move(item));
	this->dirty = true;
}

void ListBox::add(std::string &&text)
{
	this->insert(static_cast<int>(this->items.size()), std::move(text));
}

void ListBox::setText(int index, const std::string &text)
{
	DebugAssertIndex(this->items, index);
	this->items[index].text = text;
	this->dirty = true;
}

void ListBox::setOverrideColor(int index, const std::optional<Color> &overrideColor)
{
	DebugAssertIndex(this->items, index);
	this->items[index].overrideColor = overrideColor;
	this->dirty = true;
}

void ListBox::setCallback(int index, const ListBoxItemCallback &callback)
{
	DebugAssertIndex(this->items, index);
	this->items[index].callback = callback;
}

void ListBox::remove(int index)
{
	DebugAssertIndex(this->items, index);
	this->items.erase(this->items.begin() + index);
	this->dirty = true;
}

void ListBox::removeAll()
{
	this->items.clear();
	this->scrollPixelOffset = 0.0;
	this->dirty = true;
}

void ListBox::scrollDown()
{
	const int itemCount = this->getCount();
	const int itemHeightSum = this->properties.itemHeight * itemCount;
	const int itemPaddingSum = this->properties.itemSpacing * std::max(0, itemCount - 1);
	const int totalItemSizeSum = itemHeightSum + itemPaddingSum;
	const int textureHeight = this->textureRef.getHeight();
	const double maxScrollPixelOffset = std::max(0.0, static_cast<double>(totalItemSizeSum - textureHeight));

	this->scrollPixelOffset = std::min(this->scrollPixelOffset + this->getScrollDeltaPixels(), maxScrollPixelOffset);
	this->dirty = true;
}

void ListBox::scrollUp()
{
	this->scrollPixelOffset = std::max(0.0, this->scrollPixelOffset - this->getScrollDeltaPixels());
	this->dirty = true;
}

void ListBox::updateTexture()
{
	if (!this->dirty)
	{
		return;
	}

	uint32_t *texels = this->textureRef.lockTexels();
	if (texels == nullptr)
	{
		DebugLogError("Couldn't lock list box texture for updating.");
		return;
	}

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const FontDefinition &fontDef = fontLibrary.getDefinition(this->properties.fontDefIndex);

	const int width = this->textureRef.getWidth();
	const int height = this->textureRef.getHeight();
	BufferView2D<uint32_t> textureView(texels, width, height);

	// Clear texture.
	textureView.fill(0);

	// Re-draw list box items relative to where they should be with the current scroll value.
	for (int i = 0; i < static_cast<int>(this->items.size()); i++)
	{
		const ListBoxItem &item = this->items[i];
		const Rect itemRect = this->getItemLocalRect(i);
		const Color &itemColor = item.overrideColor.has_value() ? *item.overrideColor : this->properties.defaultColor;
		constexpr TextRenderColorOverrideInfo *colorOverrideInfo = nullptr;
		constexpr TextRenderShadowInfo *shadowInfo = nullptr;
		TextRenderUtils::drawTextLine(item.text, fontDef, itemRect.getLeft(), itemRect.getTop(),
			itemColor, colorOverrideInfo, shadowInfo, textureView);
	}

	this->textureRef.unlockTexels();
	this->dirty = false;
}
