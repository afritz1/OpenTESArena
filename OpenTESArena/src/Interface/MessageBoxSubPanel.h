#ifndef MESSAGE_BOX_SUB_PANEL_H
#define MESSAGE_BOX_SUB_PANEL_H

#include <functional>
#include <optional>
#include <string>

#include "SDL.h"

#include "Panel.h"
#include "../Assets/TextureUtils.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

#include "components/utilities/Buffer.h"

class FontLibrary;

struct Rect;

struct MessageBoxBackgroundProperties
{
	TextureUtils::PatternType patternType;
	int extraTitleWidth, extraTitleHeight;
	std::optional<int> widthOverride, heightOverride; // In case the texture is independent of the title text.
	int itemTextureHeight; // Width is driven by title background texture.

	MessageBoxBackgroundProperties(TextureUtils::PatternType patternType, int extraTitleWidth, int extraTitleHeight,
		const std::optional<int> &widthOverride, const std::optional<int> &heightOverride, int itemTextureHeight);
};

struct MessageBoxTitleProperties
{
	std::string fontName;
	TextRenderUtils::TextureGenInfo textureGenInfo; // Texture dimensions, etc..
	Color textColor;
	int lineSpacing;

	MessageBoxTitleProperties(const std::string &fontName, const TextRenderUtils::TextureGenInfo &textureGenInfo,
		const Color &textColor, int lineSpacing = 0);
};

struct MessageBoxItemsProperties
{
	int count;
	std::string fontName;
	TextRenderUtils::TextureGenInfo textureGenInfo; // Texture dimensions, etc..
	Color textColor;

	MessageBoxItemsProperties(int count, const std::string &fontName, const TextRenderUtils::TextureGenInfo &textureGenInfo,
		const Color &textColor);
};

using MessageBoxItemCallback = std::function<void()>;

struct MessageBoxItem
{
	Rect backgroundTextureRect;
	ScopedUiTextureRef backgroundTextureRef;
	TextBox textBox;
	MessageBoxItemCallback callback;
	std::string inputActionName; // Empty if no hotkey for this button.
	bool isCancelButton;

	MessageBoxItem();

	void init(const Rect &backgroundTextureRect, ScopedUiTextureRef &&backgroundTextureRef, TextBox &&textBox);
};

using MessageBoxOnClosedFunction = std::function<void()>;

// A sub-panel intended for displaying text with some buttons.
// @todo: might eventually make this not a panel, so it's more like TextBox and ListBox.
// - will need to make rects and textures be public + iterable then
class MessageBoxSubPanel : public Panel
{
private:
	Rect titleBackgroundRect;
	TextBox titleTextBox;
	Buffer<MessageBoxItem> items;
	ScopedUiTextureRef titleBackgroundTextureRef, cursorTextureRef;
	MessageBoxOnClosedFunction onClosed;
public:
	MessageBoxSubPanel(Game &game);
	~MessageBoxSubPanel() override;

	bool init(const MessageBoxBackgroundProperties &backgroundProperties, const Rect &titleRect,
		const MessageBoxTitleProperties &titleProperties, const MessageBoxItemsProperties &itemsProperties,
		const MessageBoxOnClosedFunction &onClosed = MessageBoxOnClosedFunction());

	void setTitleText(const std::string_view text);
	void setItemText(int itemIndex, const std::string_view text);
	void setItemCallback(int itemIndex, const MessageBoxItemCallback &callback, bool isCancelButton);
	void setItemInputAction(int itemIndex, const std::string &inputActionName);

	void addOverrideColor(int itemIndex, int charIndex, const Color &overrideColor);
	void clearOverrideColors(int itemIndex);
};

#endif
