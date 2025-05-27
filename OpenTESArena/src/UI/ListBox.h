#ifndef LIST_BOX_H
#define LIST_BOX_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "TextRenderUtils.h"
#include "Texture.h"
#include "../Math/Rect.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Color.h"

class Renderer;

struct ListBoxProperties
{
	int fontDefIndex; // Index in font library.
	TextRenderTextureGenInfo textureGenInfo; // Texture dimensions, etc..
	int itemHeight; // Pixel height of each item.
	Color defaultColor; // Color of text unless overridden.
	double scrollScale; // Percent of item size that one scroll delta moves by.
	int itemSpacing; // Pixels between each item.

	ListBoxProperties(int fontDefIndex, const TextRenderTextureGenInfo &textureGenInfo,
		int itemHeight, const Color &defaultColor, double scrollScale, int itemSpacing = 0);
	ListBoxProperties();
};

using ListBoxItemCallback = std::function<void()>;

struct ListBoxItem
{
	std::string text;
	std::optional<Color> overrideColor;
	ListBoxItemCallback callback;

	void init(std::string &&text, const std::optional<Color> &overrideColor, const ListBoxItemCallback &callback);
};

class ListBox
{
private:
	std::vector<ListBoxItem> items;
	ListBoxProperties properties;
	ScopedUiTextureRef textureRef; // Output texture, updated upon scrolling or changing the list.
	Rect rect; // Screen position and dimensions.
	double scrollPixelOffset; // Difference in pixels between the top of the first item and the top of the texture.
	bool dirty;

	// Number of pixels scrolled with one scroll delta.
	double getScrollDeltaPixels() const;

	// Redraws the underlying texture for display.
	void updateTexture();
public:
	ListBox();

	bool init(const Rect &rect, const ListBoxProperties &properties, Renderer &renderer);

	// Gets the position and dimensions of the list box in UI space.
	const Rect &getRect() const;

	// Gets the position and dimensions of the given item in list box space.
	Rect getItemLocalRect(int index) const;

	// Gets the position and dimensions of the given item in UI space.
	Rect getItemGlobalRect(int index) const;

	int getCount() const;

	// Gets the callback (if any) from the list box item at the given index. The caller needs to see if its pointer
	// falls within the list box rect.
	const ListBoxItemCallback &getCallback(int index) const;

	int getFirstVisibleItemIndex() const;

	UiTextureID getTextureID();

	void insert(int index, std::string &&text);
	void add(std::string &&text);

	// Sets an item's text, intended for overwriting existing. Does not resize the list box texture.
	void setText(int index, const std::string &text);

	// Sets an item's override color or resets it.
	void setOverrideColor(int index, const std::optional<Color> &overrideColor);

	// Sets an item's callback function.
	void setCallback(int index, const ListBoxItemCallback &callback);

	void remove(int index);
	void removeAll();

	void scrollDown();
	void scrollUp();
};

#endif
