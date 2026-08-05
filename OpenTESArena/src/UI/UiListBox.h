#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "TextRenderUtils.h"
#include "../Input/PointerTypes.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Color.h"

enum class MouseButtonType;

struct UiListBoxInitInfo
{
	int textureWidth;
	int textureHeight;
	std::vector<int> columnPixelXOffsets;
	int itemPixelSpacing;
	std::string fontName;
	Color defaultTextColor; // Color of item text unless overridden.
	Color highlightedTextColor;
	MouseButtonTypeFlags mouseButtonFlags;
	double scrollDeltaScale; // Multiplier of item height for each scroll.

	UiListBoxInitInfo();
};

using UiListBoxItemCallback = std::function<void(MouseButtonType mouseButtonType)>;

struct UiListBoxItem
{
	std::vector<std::string> textColumns;
	std::optional<Color> overrideColor;
	UiListBoxItemCallback callback;

	UiListBoxItem();

	void init(Span<const std::string> textColumns, const std::optional<Color> &overrideColor, const UiListBoxItemCallback &callback);
	void init(const std::string &text, const std::optional<Color> &overrideColor, const UiListBoxItemCallback &callback);
};

struct UiListBox
{
	UiTextureID textureID; // Owned by UI manager.
	int textureWidth;
	int textureHeight;

	std::vector<int> columnPixelXOffsets;

	int itemPixelSpacing;
	int fontDefIndex; // Index in font library.
	Color defaultTextColor;
	Color highlightedTextColor;
	double scrollDeltaScale; // Multiplier of item height for each scroll.

	std::vector<UiListBoxItem> items;
	MouseButtonTypeFlags mouseButtonFlags; // Mouse buttons allowed to trigger callback. Defaults to left mouse button only.
	double scrollPixelOffset; // How many pixels the list box is currently scrolled down.

	int highlightedItemIndex;

	bool dirty;

	UiListBox();

	void init(UiTextureID textureID, int textureWidth, int textureHeight, Span<const int> columnPixelXOffsets, int itemPixelSpacing,
		int fontDefIndex, Color defaultTextColor, Color highlightedTextColor, MouseButtonTypeFlags mouseButtonTypeFlags, double scrollDeltaScale);

	void free(Renderer &renderer);

	// Gets the item's current Y position relative to the list box origin.
	double getItemCurrentLocalY(int index) const;

	// Number of list box pixels one scroll up/down moves by.
	double getScrollDeltaPixels() const;
};
