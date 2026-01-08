#ifndef UI_LIST_BOX_H
#define UI_LIST_BOX_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "TextRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Color.h"

struct UiListBoxInitInfo
{
	int textureWidth;
	int textureHeight;
	int itemPixelSpacing;
	std::string fontName;
	TextRenderTextureGenInfo textureGenInfo; // Texture dimensions, etc..
	Color defaultTextColor; // Color of item text unless overridden.
	double scrollDeltaScale; // Multiplier of item height for each scroll.

	UiListBoxInitInfo();
};

using UiListBoxItemCallback = std::function<void()>;

struct UiListBoxItem
{
	std::string text;
	std::optional<Color> overrideColor;
	UiListBoxItemCallback callback;

	UiListBoxItem();
};

struct UiListBox
{
	UiTextureID textureID; // Owned by UI manager.
	int textureWidth;
	int textureHeight;

	int itemPixelSpacing;
	int fontDefIndex; // Index in font library.
	Color defaultTextColor;
	double scrollDeltaScale; // Multiplier of item height for each scroll.

	std::vector<UiListBoxItem> items;
	double scrollPixelOffset; // How many pixels the list box is currently scrolled down.

	bool dirty;

	UiListBox();

	void init(UiTextureID textureID, int textureWidth, int textureHeight, int itemPixelSpacing, int fontDefIndex,
		Color defaultTextColor, double scrollDeltaScale);

	void free(Renderer &renderer);

	// Gets the item's current Y position relative to the list box origin.
	double getItemCurrentLocalY(int index) const;

	// Number of list box pixels one scroll up/down moves by.
	double getScrollDeltaPixels() const;
};

#endif
