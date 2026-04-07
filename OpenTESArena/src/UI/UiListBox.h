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
	int itemPixelSpacing;
	std::string fontName;
	Color defaultTextColor; // Color of item text unless overridden.
	MouseButtonTypeFlags mouseButtonFlags;
	double scrollDeltaScale; // Multiplier of item height for each scroll.

	UiListBoxInitInfo();
};

using UiListBoxItemCallback = std::function<void(MouseButtonType mouseButtonType)>;

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
	MouseButtonTypeFlags mouseButtonFlags; // Mouse buttons allowed to trigger callback. Defaults to left mouse button only.
	double scrollPixelOffset; // How many pixels the list box is currently scrolled down.

	bool dirty;

	UiListBox();

	void init(UiTextureID textureID, int textureWidth, int textureHeight, int itemPixelSpacing, int fontDefIndex,
		Color defaultTextColor, MouseButtonTypeFlags mouseButtonTypeFlags, double scrollDeltaScale);

	void free(Renderer &renderer);

	// Gets the item's current Y position relative to the list box origin.
	double getItemCurrentLocalY(int index) const;

	// Number of list box pixels one scroll up/down moves by.
	double getScrollDeltaPixels() const;
};
