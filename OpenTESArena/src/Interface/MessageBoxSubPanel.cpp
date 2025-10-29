#include "CommonUiView.h"
#include "MessageBoxSubPanel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Math/Rect.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"

MessageBoxBackgroundProperties::MessageBoxBackgroundProperties(UiTexturePatternType patternType,
	int extraTitleWidth, int extraTitleHeight, const std::optional<int> &widthOverride,
	const std::optional<int> &heightOverride, int itemTextureHeight)
	: widthOverride(widthOverride), heightOverride(heightOverride)
{
	this->patternType = patternType;
	this->extraTitleWidth = extraTitleWidth;
	this->extraTitleHeight = extraTitleHeight;
	this->itemTextureHeight = itemTextureHeight;
}

MessageBoxTitleProperties::MessageBoxTitleProperties(const std::string &fontName,
	const TextRenderTextureGenInfo &textureGenInfo, const Color &textColor, int lineSpacing)
	: fontName(fontName), textureGenInfo(textureGenInfo), textColor(textColor)
{
	this->lineSpacing = lineSpacing;
}

MessageBoxItemsProperties::MessageBoxItemsProperties(int count, const std::string &fontName,
	const TextRenderTextureGenInfo &textureGenInfo, const Color &textColor)
	: fontName(fontName), textureGenInfo(textureGenInfo), textColor(textColor)
{
	this->count = count;
}

MessageBoxItem::MessageBoxItem()
{
	this->isCancelButton = false;
}

void MessageBoxItem::init(const Rect &backgroundTextureRect, ScopedUiTextureRef &&backgroundTextureRef, TextBox &&textBox)
{
	this->backgroundTextureRect = backgroundTextureRect;
	this->backgroundTextureRef = std::move(backgroundTextureRef);
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

bool MessageBoxSubPanel::init(const MessageBoxBackgroundProperties &backgroundProperties, const Rect &titleRect,
	const MessageBoxTitleProperties &titleProperties, const MessageBoxItemsProperties &itemsProperties,
	const MessageBoxOnClosedFunction &onClosed)
{
	auto &game = this->getGame();
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const FontLibrary &fontLibrary = FontLibrary::getInstance();

	// The background expands to fit the text, unless overridden.
	const int titleBackgroundWidth = backgroundProperties.widthOverride.has_value() ?
		*backgroundProperties.widthOverride : (titleRect.width + backgroundProperties.extraTitleWidth);
	const int titleBackgroundHeight = backgroundProperties.heightOverride.has_value() ?
		*backgroundProperties.heightOverride : (titleRect.height + backgroundProperties.extraTitleHeight);
	this->titleBackgroundRect = Rect(titleRect.getCenter(), titleBackgroundWidth, titleBackgroundHeight);

	const Surface titleBackgroundSurface = TextureUtils::generate(backgroundProperties.patternType,
		this->titleBackgroundRect.width, this->titleBackgroundRect.height, textureManager, renderer);

	UiTextureID titleBackgroundTextureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(titleBackgroundSurface, textureManager, renderer, &titleBackgroundTextureID))
	{
		DebugLogError("Couldn't create title background texture from surface.");
		return false;
	}

	this->titleBackgroundTextureRef.init(titleBackgroundTextureID, renderer);

	int titleFontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(titleProperties.fontName.c_str(), &titleFontDefIndex))
	{
		DebugLogError("Couldn't get message box title font definition for \"" + titleProperties.fontName + "\".");
		return false;
	}

	constexpr TextAlignment titleAlignment = TextAlignment::MiddleCenter;
	const TextBoxProperties titleTextBoxProperties(titleFontDefIndex, titleProperties.textureGenInfo, titleProperties.textColor,
		titleAlignment, std::nullopt, titleProperties.lineSpacing);
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
	const TextBoxProperties itemTextBoxProperties(itemFontDefIndex, itemsProperties.textureGenInfo, itemsProperties.textColor, itemAlignment);

	this->items.init(itemsProperties.count);
	for (int i = 0; i < this->items.getCount(); i++)
	{
		const Rect itemBackgroundRect(
			titleBackgroundRect.getLeft(),
			titleBackgroundRect.getBottom() + (i * backgroundProperties.itemTextureHeight),
			titleBackgroundRect.width,
			backgroundProperties.itemTextureHeight);

		const Surface itemBackgroundSurface = TextureUtils::generate(backgroundProperties.patternType,
			itemBackgroundRect.width, itemBackgroundRect.height, textureManager, renderer);
		
		UiTextureID itemBackgroundTextureID;
		if (!TextureUtils::tryAllocUiTextureFromSurface(itemBackgroundSurface, textureManager, renderer, &itemBackgroundTextureID))
		{
			DebugLogError("Couldn't create item background " + std::to_string(i) + " texture from surface.");
			return false;
		}

		ScopedUiTextureRef itemBackgroundTextureRef(itemBackgroundTextureID, renderer);

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

		MessageBoxItem &item = this->items.get(i);
		item.init(itemBackgroundRect, std::move(itemBackgroundTextureRef), std::move(itemTextBox));

		this->addButtonProxy(MouseButtonType::Left, itemBackgroundRect,
			[this, i]()
		{
			MessageBoxItem &item = this->items.get(i);
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
				const MessageBoxItem &item = this->items.get(i);
				if (item.isCancelButton)
				{
					item.callback();
					break;
				}
			}
		}
	});

	UiDrawCallInitInfo titleBgDrawCallInitInfo;
	titleBgDrawCallInitInfo.textureID = this->titleBackgroundTextureRef.get();
	titleBgDrawCallInitInfo.position = this->titleBackgroundRect.getCenter();
	titleBgDrawCallInitInfo.size = this->titleBackgroundRect.getSize();
	titleBgDrawCallInitInfo.pivotType = PivotType::Middle;
	this->addDrawCall(titleBgDrawCallInitInfo);

	for (const MessageBoxItem &item : this->items)
	{
		UiDrawCallInitInfo itemBgDrawCallInitInfo;
		itemBgDrawCallInitInfo.textureID = item.backgroundTextureRef.get();
		itemBgDrawCallInitInfo.position = item.backgroundTextureRect.getCenter();
		itemBgDrawCallInitInfo.size = item.backgroundTextureRect.getSize();
		itemBgDrawCallInitInfo.pivotType = PivotType::Middle;
		this->addDrawCall(itemBgDrawCallInitInfo);
	}

	const Rect titleTextBoxRect = this->titleTextBox.getRect();
	UiDrawCallInitInfo titleTextDrawCallInitInfo;
	titleTextDrawCallInitInfo.textureFunc = [this]() { return this->titleTextBox.getTextureID(); };
	titleTextDrawCallInitInfo.position = titleTextBoxRect.getCenter();
	titleTextDrawCallInitInfo.size = titleTextBoxRect.getSize();
	titleTextDrawCallInitInfo.pivotType = PivotType::Middle;
	this->addDrawCall(titleTextDrawCallInitInfo);

	for (int i = 0; i < this->items.getCount(); i++)
	{
		UiDrawCallInitInfo itemTextDrawCallInitInfo;
		itemTextDrawCallInitInfo.textureFunc = [this, i]()
		{
			MessageBoxItem &item = this->items.get(i);
			return item.textBox.getTextureID();
		};

		itemTextDrawCallInitInfo.positionFunc = [this, i]()
		{
			MessageBoxItem &item = this->items.get(i);
			const Rect &itemRect = item.textBox.getRect();
			return itemRect.getCenter();
		};

		itemTextDrawCallInitInfo.sizeFunc = [this, i]()
		{
			MessageBoxItem &item = this->items.get(i);
			const Rect &itemRect = item.textBox.getRect();
			return itemRect.getSize();
		};

		itemTextDrawCallInitInfo.pivotType = PivotType::Middle;
		this->addDrawCall(itemTextDrawCallInitInfo);
	}

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft);

	this->onClosed = onClosed;

	return true;
}

void MessageBoxSubPanel::setTitleText(const std::string_view text)
{
	this->titleTextBox.setText(text);
}

void MessageBoxSubPanel::setItemText(int itemIndex, const std::string_view text)
{
	MessageBoxItem &item = this->items.get(itemIndex);
	item.textBox.setText(text);
}

void MessageBoxSubPanel::setItemCallback(int itemIndex, const MessageBoxItemCallback &callback, bool isCancelButton)
{
	MessageBoxItem &item = this->items.get(itemIndex);
	item.callback = callback;
	item.isCancelButton = isCancelButton;
}

void MessageBoxSubPanel::setItemInputAction(int itemIndex, const std::string &inputActionName)
{
	DebugAssert(!inputActionName.empty());

	MessageBoxItem &item = this->items.get(itemIndex);

	// Only support setting the hotkey once due to the complication of finding and removing old input actions.
	DebugAssert(item.inputActionName.empty());
	item.inputActionName = inputActionName;

	this->addInputActionListener(inputActionName,
		[this, itemIndex](const InputActionCallbackValues &values)
	{
		MessageBoxItem &item = this->items.get(itemIndex);
		DebugAssert(item.callback);
		item.callback();
	});
}

void MessageBoxSubPanel::addOverrideColor(int itemIndex, int charIndex, const Color &overrideColor)
{
	MessageBoxItem &item = this->items.get(itemIndex);
	item.textBox.addOverrideColor(charIndex, overrideColor);
}

void MessageBoxSubPanel::clearOverrideColors(int itemIndex)
{
	MessageBoxItem &item = this->items.get(itemIndex);
	item.textBox.clearOverrideColors();
}
