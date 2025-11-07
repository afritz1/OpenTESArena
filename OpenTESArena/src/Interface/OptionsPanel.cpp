#include <algorithm>
#include <string>

#include "CommonUiView.h"
#include "OptionsPanel.h"
#include "OptionsUiController.h"
#include "OptionsUiModel.h"
#include "OptionsUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"

OptionsPanel::OptionsPanel(Game &game)
	: Panel(game) { }

bool OptionsPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const TextBoxInitInfo descTextBoxInitInfo = OptionsUiView::getDescriptionTextBoxInitInfo(fontLibrary);
	if (!this->descriptionTextBox.init(descTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init description text box.");
		return false;
	}

	const std::string backButtonText = OptionsUiModel::BackButtonText;
	const TextBoxInitInfo backButtonTextBoxInitInfo = OptionsUiView::getBackButtonTextBoxInitInfo(backButtonText, fontLibrary);
	if (!this->backButtonTextBox.init(backButtonTextBoxInitInfo, backButtonText, renderer))
	{
		DebugLogError("Couldn't init back button text box.");
		return false;
	}

	for (int i = 0; i < OptionsUiModel::TAB_COUNT; i++)
	{
		const std::string &tabName = OptionsUiModel::TAB_NAMES[i];
		const TextBoxInitInfo tabInitInfo = OptionsUiView::getTabTextBoxInitInfo(i, tabName, fontLibrary);
		TextBox tabTextBox;
		if (!tabTextBox.init(tabInitInfo, tabName, renderer))
		{
			DebugLogError("Couldn't init tab text box " + std::to_string(i) + ".");
			continue;
		}

		this->tabTextBoxes.emplace_back(std::move(tabTextBox));
	}

	for (int i = 0; i < OptionsUiModel::OPTION_COUNT; i++)
	{
		// Initialize to blank -- the text box will be populated later by the current tab.
		const TextBoxInitInfo initInfo = OptionsUiView::getOptionTextBoxInitInfo(i, fontLibrary);
		TextBox textBox;
		if (!textBox.init(initInfo, renderer))
		{
			DebugLogError("Couldn't init option text box " + std::to_string(i) + ".");
			continue;
		}

		this->optionTextBoxes.emplace_back(std::move(textBox));
	}

	// Button proxies are added later.
	this->backButton = Button<Game&>(OptionsUiView::getBackButtonRect(), OptionsUiController::onBackButtonSelected);
	this->tabButton = Button<OptionsPanel&, OptionsUiModel::Tab*, OptionsUiModel::Tab>(OptionsUiController::onTabButtonSelected);

	this->addInputActionListener(InputActionName::Back,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->backButton.click(values.game);
		}
	});

	auto updateHoveredOptionIndex = [this, &game]()
	{
		// Update description if over a valid element.
		const std::optional<int> hoveredIndex = this->getHoveredOptionIndex();
		if (hoveredIndex != this->hoveredOptionIndex)
		{
			this->hoveredOptionIndex = hoveredIndex;

			const auto &visibleOptions = this->getVisibleOptions();
			if (hoveredIndex.has_value() && (*hoveredIndex < static_cast<int>(visibleOptions.size())))
			{
				const auto &option = visibleOptions[*hoveredIndex];
				const std::string &descText = option->tooltip;
				this->descriptionTextBox.setText(descText);
			}
			else
			{
				this->descriptionTextBox.setText(std::string());
			}
		}
	};

	this->addMouseMotionListener([this, updateHoveredOptionIndex](Game &game, int dx, int dy)
	{
		updateHoveredOptionIndex();
	});

	TextureManager &textureManager = game.textureManager;
	const UiTextureID backgroundTextureID = OptionsUiView::allocBackgroundTexture(renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);

	UiDrawCallInitInfo bgDrawCallInitInfo;
	bgDrawCallInitInfo.textureID = this->backgroundTextureRef.get();
	bgDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(bgDrawCallInitInfo);

	const UiTextureID tabTextureID = OptionsUiView::allocTabTexture(textureManager, renderer);
	this->tabButtonTextureRef.init(tabTextureID, renderer);
	for (int i = 0; i < OptionsUiModel::TAB_COUNT; i++)
	{
		const Rect tabRect = OptionsUiView::getTabRect(i);
		UiDrawCallInitInfo tabTextureDrawCallInitInfo;
		tabTextureDrawCallInitInfo.textureID = this->tabButtonTextureRef.get();
		tabTextureDrawCallInitInfo.position = tabRect.getTopLeft();
		tabTextureDrawCallInitInfo.size = tabRect.getSize();
		this->addDrawCall(tabTextureDrawCallInitInfo);
	}

	const UiTextureID backButtonTextureID = OptionsUiView::allocBackButtonTexture(textureManager, renderer);
	this->backButtonTextureRef.init(backButtonTextureID, renderer);
	const Rect backButtonRect = OptionsUiView::getBackButtonRect();
	UiDrawCallInitInfo backTextureDrawCallInitInfo;
	backTextureDrawCallInitInfo.textureID = this->backButtonTextureRef.get();
	backTextureDrawCallInitInfo.position = backButtonRect.getTopLeft();
	backTextureDrawCallInitInfo.size = backButtonRect.getSize();
	this->addDrawCall(backTextureDrawCallInitInfo);

	const Rect backButtonTextBoxRect = this->backButtonTextBox.getRect();
	UiDrawCallInitInfo backTextDrawCallInitInfo;
	backTextDrawCallInitInfo.textureID = backButtonTextBox.getTextureID();
	backTextDrawCallInitInfo.position = backButtonTextBoxRect.getTopLeft();
	backTextDrawCallInitInfo.size = backButtonTextBoxRect.getSize();
	this->addDrawCall(backTextDrawCallInitInfo);

	const UiTextureID highlightTextureID = OptionsUiView::allocHighlightTexture(renderer);
	this->highlightTextureRef.init(highlightTextureID, renderer);
	for (int i = 0; i < OptionsUiModel::OPTION_COUNT; i++)
	{
		DebugAssertIndex(this->optionTextBoxes, i);
		const TextBox &textBox = this->optionTextBoxes[i];
		const Rect textBoxRect = textBox.getRect();

		UiDrawCallInitInfo highlightDrawCallInitInfo;
		highlightDrawCallInitInfo.textureID = this->highlightTextureRef.get();
		highlightDrawCallInitInfo.position = textBoxRect.getTopLeft();
		highlightDrawCallInitInfo.size = textBoxRect.getSize();
		highlightDrawCallInitInfo.activeFunc = [this, i]()
		{
			const OptionsUiModel::OptionGroup &visibleOptions = this->getVisibleOptions();
			if (i >= static_cast<int>(visibleOptions.size()))
			{
				return false;
			}

			const Game &game = this->getGame();
			const Window &window = game.window;
			const InputManager &inputManager = game.inputManager;
			const Int2 mousePosition = inputManager.getMousePosition();
			const Int2 originalPoint = window.nativeToOriginal(mousePosition);

			DebugAssertIndex(this->optionTextBoxes, i);
			const TextBox &textBox = this->optionTextBoxes[i];
			const Rect &textBoxRect = textBox.getRect();
			return textBoxRect.contains(originalPoint);
		};
		
		this->addDrawCall(highlightDrawCallInitInfo);
	}

	for (TextBox &tabTextBox : this->tabTextBoxes)
	{
		const Rect tabTextBoxRect = tabTextBox.getRect();
		UiDrawCallInitInfo tabTextDrawCallInitInfo;
		tabTextDrawCallInitInfo.textureID = tabTextBox.getTextureID();
		tabTextDrawCallInitInfo.position = tabTextBoxRect.getTopLeft();
		tabTextDrawCallInitInfo.size = tabTextBoxRect.getSize();
		this->addDrawCall(tabTextDrawCallInitInfo);
	}

	for (int i = 0; i < static_cast<int>(this->optionTextBoxes.size()); i++)
	{
		const TextBox &optionTextBox = this->optionTextBoxes[i];
		const Rect optionTextBoxRect = optionTextBox.getRect();

		UiDrawCallInitInfo optionTextDrawCallInitInfo;
		optionTextDrawCallInitInfo.textureFunc = [this, i]()
		{
			DebugAssertIndex(this->optionTextBoxes, i);
			TextBox &optionTextBox = this->optionTextBoxes[i];
			return optionTextBox.getTextureID();
		};

		optionTextDrawCallInitInfo.position = optionTextBoxRect.getTopLeft();
		optionTextDrawCallInitInfo.size = optionTextBoxRect.getSize();		
		this->addDrawCall(optionTextDrawCallInitInfo);
	}

	const Rect descTextBoxRect = this->descriptionTextBox.getRect();
	UiDrawCallInitInfo descriptionTextDrawCallInitInfo;
	descriptionTextDrawCallInitInfo.textureFunc = [this]() { return this->descriptionTextBox.getTextureID(); };
	descriptionTextDrawCallInitInfo.position = descTextBoxRect.getTopLeft();
	descriptionTextDrawCallInitInfo.size = descTextBoxRect.getSize();
	this->addDrawCall(descriptionTextDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), UiPivotType::TopLeft);

	// Create option groups.
	for (int i = 0; i < OptionsUiModel::TAB_COUNT; i++)
	{
		OptionsUiModel::OptionGroup group = OptionsUiModel::makeOptionGroup(i, game);
		this->optionGroups.emplace_back(std::move(group));
	}

	// Populate option text boxes and option buttons for the initial tab.
	this->tab = OptionsUiModel::Tab::Graphics;
	this->updateVisibleOptions();

	updateHoveredOptionIndex();

	return true;
}

OptionsUiModel::OptionGroup &OptionsPanel::getVisibleOptions()
{
	const int index = static_cast<int>(this->tab);
	DebugAssertIndex(this->optionGroups, index);
	return this->optionGroups[index];
}

std::optional<int> OptionsPanel::getHoveredOptionIndex() const
{
	const Game &game = this->getGame();
	const Window &window = game.window;
	const InputManager &inputManager = game.inputManager;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = window.nativeToOriginal(mousePosition);

	for (int i = 0; i < static_cast<int>(this->optionTextBoxes.size()); i++)
	{
		const TextBox &optionTextBox = this->optionTextBoxes[i];
		const Rect &optionRect = optionTextBox.getRect();
		if (optionRect.contains(originalPoint))
		{
			return i;
		}
	}

	return std::nullopt;
}

void OptionsPanel::updateOptionText(int index)
{
	auto &game = this->getGame();
	const auto &visibleOptions = this->getVisibleOptions();
	DebugAssertIndex(visibleOptions, index);
	const auto &visibleOption = visibleOptions[index];
	const std::string text = visibleOption->name + ": " + visibleOption->getDisplayedValue();

	DebugAssertIndex(this->optionTextBoxes, index);
	TextBox &textBox = this->optionTextBoxes[index];
	textBox.setText(text);
}

void OptionsPanel::updateVisibleOptions()
{
	auto &game = this->getGame();

	// Remove all button proxies, including the static ones.
	this->clearButtonProxies();

	auto addTabButtonProxy = [this](int index)
	{
		const OptionsUiModel::Tab tab = static_cast<OptionsUiModel::Tab>(index);
		const Rect rect = OptionsUiView::getTabRect(index);
		this->addButtonProxy(MouseButtonType::Left, rect,
			[this, tab]() { this->tabButton.click(*this, &this->tab, tab); });
	};

	// Add the static button proxies.
	for (int i = 0; i < OptionsUiModel::TAB_COUNT; i++)
	{
		addTabButtonProxy(i);
	}

	this->addButtonProxy(MouseButtonType::Left, this->backButton.getRect(),
		[this, &game]() { this->backButton.click(game); });

	auto addOptionButtonProxies = [this](int index)
	{
		auto buttonFunc = [this, index](bool isLeftClick)
		{
			auto &visibleOptions = this->getVisibleOptions();
			DebugAssertIndex(visibleOptions, index);
			std::unique_ptr<OptionsUiModel::Option> &option = visibleOptions[index];

			if (isLeftClick)
			{
				option->tryIncrement();
			}
			else
			{
				option->tryDecrement();
			}

			this->updateOptionText(index);
		};

		DebugAssertIndex(this->optionTextBoxes, index);
		const TextBox &optionTextBox = this->optionTextBoxes[index];
		const Rect optionRect = optionTextBox.getRect();

		this->addButtonProxy(MouseButtonType::Left, optionRect, [buttonFunc]() { buttonFunc(true); });
		this->addButtonProxy(MouseButtonType::Right, optionRect, [buttonFunc]() { buttonFunc(false); });
	};

	// Clear option text boxes and then re-populate the visible ones.
	for (TextBox &textBox : this->optionTextBoxes)
	{
		textBox.setText(std::string());
	}

	const auto &visibleOptions = this->getVisibleOptions();
	for (int i = 0; i < static_cast<int>(visibleOptions.size()); i++)
	{
		this->updateOptionText(i);
		addOptionButtonProxies(i);
	}
}
