#ifndef MESSAGE_BOX_SUB_PANEL_H
#define MESSAGE_BOX_SUB_PANEL_H

#include <functional>
#include <optional>
#include <string>

#include "Panel.h"
#include "../Media/TextureUtils.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

#include "components/utilities/Buffer.h"

// A sub-panel intended for displaying text with some buttons.

// @todo: might eventually make this not a panel, so it's more like TextBox and ListBox.
// - will need to make rects and textures be public + iterable then

class FontLibrary;
class Rect;

class MessageBoxSubPanel : public Panel
{
public:
	struct BackgroundProperties
	{
		TextureUtils::PatternType patternType;
		int extraTitleWidth, extraTitleHeight;
		std::optional<int> widthOverride, heightOverride; // In case the texture is independent of the title text.
		int itemTextureHeight; // Width is driven by title background texture.

		BackgroundProperties(TextureUtils::PatternType patternType, int extraTitleWidth, int extraTitleHeight,
			const std::optional<int> &widthOverride, const std::optional<int> &heightOverride, int itemTextureHeight);
	};

	struct TitleProperties
	{
		std::string fontName;
		TextRenderUtils::TextureGenInfo textureGenInfo; // Texture dimensions, etc..
		Color textColor;
		int lineSpacing;

		TitleProperties(const std::string &fontName, const TextRenderUtils::TextureGenInfo &textureGenInfo,
			const Color &textColor, int lineSpacing = 0);
	};

	struct ItemsProperties
	{
		int count;
		std::string fontName;
		TextRenderUtils::TextureGenInfo textureGenInfo; // Texture dimensions, etc..
		Color textColor;

		ItemsProperties(int count, const std::string &fontName, const TextRenderUtils::TextureGenInfo &textureGenInfo,
			const Color &textColor);
	};

	using ItemCallback = std::function<void()>;
private:
	struct Item
	{
		Rect backgroundTextureRect;
		Texture backgroundTexture;
		TextBox textBox;
		ItemCallback callback;
		bool isCancelButton;

		Item();

		void init(const Rect &backgroundTextureRect, Texture &&backgroundTexture, TextBox &&textBox);
	};

	Rect titleBackgroundRect;
	Texture titleBackgroundTexture;
	TextBox titleTextBox;
	Buffer<Item> items;
public:
	MessageBoxSubPanel(Game &game);
	~MessageBoxSubPanel() override = default;

	bool init(const BackgroundProperties &backgroundProperties, const Rect &titleRect,
		const TitleProperties &titleProperties, const ItemsProperties &itemsProperties);

	void setTitleText(const std::string_view &text);
	void setItemText(int itemIndex, const std::string_view &text);
	void setItemCallback(int itemIndex, const ItemCallback &callback, bool isCancelButton);

	void addOverrideColor(int itemIndex, int charIndex, const Color &overrideColor);
	void clearOverrideColors(int itemIndex);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
