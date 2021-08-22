#include "MessageBoxSubPanel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Math/Rect.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/TextAlignment.h"

MessageBoxSubPanel::BackgroundProperties::BackgroundProperties(TextureUtils::PatternType patternType,
	int extraTitleWidth, int extraTitleHeight, const std::optional<int> &widthOverride,
	const std::optional<int> &heightOverride, int itemTextureHeight)
	: widthOverride(widthOverride), heightOverride(heightOverride)
{
	this->patternType = patternType;
	this->extraTitleWidth = extraTitleWidth;
	this->extraTitleHeight = extraTitleHeight;
	this->itemTextureHeight = itemTextureHeight;
}

MessageBoxSubPanel::TitleProperties::TitleProperties(const std::string &fontName,
	const TextRenderUtils::TextureGenInfo &textureGenInfo, const Color &textColor, int lineSpacing)
	: fontName(fontName), textureGenInfo(textureGenInfo), textColor(textColor)
{
	this->lineSpacing = lineSpacing;
}

MessageBoxSubPanel::ItemsProperties::ItemsProperties(int count, const std::string &fontName,
	const TextRenderUtils::TextureGenInfo &textureGenInfo, const Color &textColor)
	: fontName(fontName), textureGenInfo(textureGenInfo), textColor(textColor)
{
	this->count = count;
}

MessageBoxSubPanel::Item::Item()
{
	this->isCancelButton = false;
}

void MessageBoxSubPanel::Item::init(const Rect &backgroundTextureRect, Texture &&backgroundTexture, TextBox &&textBox)
{
	this->backgroundTextureRect = backgroundTextureRect;
	this->backgroundTexture = std::move(backgroundTexture);
	this->textBox = std::move(textBox);
}

MessageBoxSubPanel::MessageBoxSubPanel(Game &game)
	: Panel(game) { }

MessageBoxSubPanel::~MessageBoxSubPanel()
{
	if (this->onClosed)
	{
		this->onClosed();
	}
}

bool MessageBoxSubPanel::init(const BackgroundProperties &backgroundProperties, const Rect &titleRect,
	const TitleProperties &titleProperties, const ItemsProperties &itemsProperties,
	const OnClosedFunction &onClosed)
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const FontLibrary &fontLibrary = game.getFontLibrary();

	// The background expands to fit the text, unless overridden.
	const int titleBackgroundWidth = backgroundProperties.widthOverride.has_value() ?
		*backgroundProperties.widthOverride : (titleRect.getWidth() + backgroundProperties.extraTitleWidth);
	const int titleBackgroundHeight = backgroundProperties.heightOverride.has_value() ?
		*backgroundProperties.heightOverride : (titleRect.getHeight() + backgroundProperties.extraTitleHeight);
	this->titleBackgroundRect = Rect(titleRect.getCenter(), titleBackgroundWidth, titleBackgroundHeight);

	this->titleBackgroundTexture = TextureUtils::generate(backgroundProperties.patternType,
		this->titleBackgroundRect.getWidth(), this->titleBackgroundRect.getHeight(), textureManager, renderer);
	if (this->titleBackgroundTexture.get() == nullptr)
	{
		DebugLogError("Couldn't init message box title background texture.");
		return false;
	}

	int titleFontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(titleProperties.fontName.c_str(), &titleFontDefIndex))
	{
		DebugLogError("Couldn't get message box title font definition for \"" + titleProperties.fontName + "\".");
		return false;
	}

	constexpr TextAlignment titleAlignment = TextAlignment::MiddleCenter;
	const TextBox::Properties titleTextBoxProperties(titleFontDefIndex, &fontLibrary, titleProperties.textureGenInfo,
		titleProperties.textColor, titleAlignment, std::nullopt, titleProperties.lineSpacing);
	if (!this->titleTextBox.init(titleRect, titleTextBoxProperties, renderer))
	{
		DebugLogError("Couldn't init message box title text box.");
		return false;
	}

	int itemFontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(itemsProperties.fontName.c_str(), &itemFontDefIndex))
	{
		DebugLogError("Couldn't get message box item font definition for \"" + itemsProperties.fontName + "\".");
		return false;
	}

	constexpr TextAlignment itemAlignment = titleAlignment;
	const TextBox::Properties itemTextBoxProperties(itemFontDefIndex, &fontLibrary, itemsProperties.textureGenInfo,
		itemsProperties.textColor, itemAlignment);

	this->items.init(itemsProperties.count);
	for (int i = 0; i < this->items.getCount(); i++)
	{
		const Rect itemBackgroundRect(
			titleBackgroundRect.getLeft(),
			titleBackgroundRect.getBottom() + (i * backgroundProperties.itemTextureHeight),
			titleBackgroundRect.getWidth(),
			backgroundProperties.itemTextureHeight);

		Texture itemBackgroundTexture = TextureUtils::generate(backgroundProperties.patternType,
			itemBackgroundRect.getWidth(), itemBackgroundRect.getHeight(), textureManager, renderer);

		Rect itemTextBoxRect(
			itemBackgroundRect.getCenter(),
			itemsProperties.textureGenInfo.width,
			itemsProperties.textureGenInfo.height);

		TextBox itemTextBox;
		if (!itemTextBox.init(itemTextBoxRect, itemTextBoxProperties, renderer))
		{
			DebugLogError("Couldn't init message box item text box " + std::to_string(i) + ".");
			return false;
		}

		MessageBoxSubPanel::Item &item = this->items.get(i);
		item.init(itemBackgroundRect, std::move(itemBackgroundTexture), std::move(itemTextBox));

		this->addButtonProxy(MouseButtonType::Left, itemBackgroundRect,
			[this, i]()
		{
			MessageBoxSubPanel::Item &item = this->items.get(i);
			DebugAssert(item.callback);
			item.callback();
		});
	}

	this->addInputActionListener(InputActionName::Back,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			// Try to close the message box as if a cancel button had been clicked.
			for (int i = 0; i < this->items.getCount(); i++)
			{
				const MessageBoxSubPanel::Item &item = this->items.get(i);
				if (item.isCancelButton)
				{
					item.callback();
					break;
				}
			}
		}
	});

	this->onClosed = onClosed;

	return true;
}

void MessageBoxSubPanel::setTitleText(const std::string_view &text)
{
	this->titleTextBox.setText(text);
}

void MessageBoxSubPanel::setItemText(int itemIndex, const std::string_view &text)
{
	MessageBoxSubPanel::Item &item = this->items.get(itemIndex);
	item.textBox.setText(text);
}

void MessageBoxSubPanel::setItemCallback(int itemIndex, const ItemCallback &callback, bool isCancelButton)
{
	MessageBoxSubPanel::Item &item = this->items.get(itemIndex);
	item.callback = callback;
	item.isCancelButton = isCancelButton;
}

void MessageBoxSubPanel::setItemInputAction(int itemIndex, const std::string &inputActionName)
{
	DebugAssert(!inputActionName.empty());

	MessageBoxSubPanel::Item &item = this->items.get(itemIndex);

	// Only support setting the hotkey once due to the complication of finding and removing old input actions.
	DebugAssert(item.inputActionName.empty());
	item.inputActionName = inputActionName;

	this->addInputActionListener(inputActionName,
		[this, itemIndex](const InputActionCallbackValues &values)
	{
		MessageBoxSubPanel::Item &item = this->items.get(itemIndex);
		DebugAssert(item.callback);
		item.callback();
	});
}

void MessageBoxSubPanel::addOverrideColor(int itemIndex, int charIndex, const Color &overrideColor)
{
	MessageBoxSubPanel::Item &item = this->items.get(itemIndex);
	item.textBox.addOverrideColor(charIndex, overrideColor);
}

void MessageBoxSubPanel::clearOverrideColors(int itemIndex)
{
	MessageBoxSubPanel::Item &item = this->items.get(itemIndex);
	item.textBox.clearOverrideColors();
}

std::optional<CursorData> MessageBoxSubPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();

	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return std::nullopt;
	}

	const std::string &textureFilename = ArenaTextureName::SwordCursor;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogWarning("Couldn't get texture builder ID for \"" + textureFilename + "\".");
		return std::nullopt;
	}

	return CursorData(*textureBuilderID, *paletteID, CursorAlignment::TopLeft);
}

void MessageBoxSubPanel::render(Renderer &renderer)
{
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	renderer.drawOriginal(this->titleBackgroundTexture, this->titleBackgroundRect.getLeft(), this->titleBackgroundRect.getTop());
	renderer.drawOriginal(this->titleTextBox.getTextureID(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());

	for (int i = 0; i < this->items.getCount(); i++)
	{
		MessageBoxSubPanel::Item &item = this->items.get(i);
		const Rect &itemBackgroundRect = item.backgroundTextureRect;
		const Rect &itemTextBoxRect = item.textBox.getRect();
		renderer.drawOriginal(item.backgroundTexture, itemBackgroundRect.getLeft(), itemBackgroundRect.getTop());
		renderer.drawOriginal(item.textBox.getTextureID(), itemTextBoxRect.getLeft(), itemTextBoxRect.getTop());
	}
}
