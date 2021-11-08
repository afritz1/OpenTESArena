#include <algorithm>
#include <string>

#include "CommonUiView.h"
#include "OptionsPanel.h"
#include "OptionsUiController.h"
#include "OptionsUiModel.h"
#include "OptionsUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"

OptionsPanel::OptionsPanel(Game &game)
	: Panel(game) { }

bool OptionsPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string titleText = OptionsUiModel::TitleText;
	const TextBox::InitInfo titleTextBoxInitInfo = OptionsUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBox::InitInfo descTextBoxInitInfo = OptionsUiView::getDescriptionTextBoxInitInfo(fontLibrary);
	if (!this->descriptionTextBox.init(descTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init description text box.");
		return false;
	}

	const std::string backButtonText = OptionsUiModel::BackButtonText;
	const TextBox::InitInfo backButtonTextBoxInitInfo = OptionsUiView::getBackButtonTextBoxInitInfo(backButtonText, fontLibrary);
	if (!this->backButtonTextBox.init(backButtonTextBoxInitInfo, backButtonText, renderer))
	{
		DebugLogError("Couldn't init back button text box.");
		return false;
	}

	for (int i = 0; i < OptionsUiModel::TAB_COUNT; i++)
	{
		const std::string &tabName = OptionsUiModel::TAB_NAMES[i];
		const TextBox::InitInfo tabInitInfo = OptionsUiView::getTabTextBoxInitInfo(i, tabName, fontLibrary);
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
		const TextBox::InitInfo initInfo = OptionsUiView::getOptionTextBoxInitInfo(i, fontLibrary);
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
				const std::string &descText = option->getTooltip();
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

	auto &textureManager = game.getTextureManager();
	const UiTextureID backgroundTextureID = OptionsUiView::allocBackgroundTexture(renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);
	this->addDrawCall(
		this->backgroundTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

	const UiTextureID tabTextureID = OptionsUiView::allocTabTexture(textureManager, renderer);
	this->tabButtonTextureRef.init(tabTextureID, renderer);
	for (int i = 0; i < OptionsUiModel::TAB_COUNT; i++)
	{
		const Rect tabRect = OptionsUiView::getTabRect(i);
		this->addDrawCall(
			this->tabButtonTextureRef.get(),
			tabRect.getTopLeft(),
			Int2(tabRect.getWidth(), tabRect.getHeight()),
			PivotType::TopLeft);
	}

	const UiTextureID backButtonTextureID = OptionsUiView::allocBackButtonTexture(textureManager, renderer);
	this->backButtonTextureRef.init(backButtonTextureID, renderer);
	const Rect backButtonRect = OptionsUiView::getBackButtonRect();
	this->addDrawCall(
		this->backButtonTextureRef.get(),
		backButtonRect.getTopLeft(),
		Int2(backButtonRect.getWidth(), backButtonRect.getHeight()),
		PivotType::TopLeft);

	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	this->addDrawCall(
		titleTextBox.getTextureID(),
		titleTextBoxRect.getTopLeft(),
		Int2(titleTextBoxRect.getWidth(), titleTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &backButtonTextBoxRect = this->backButtonTextBox.getRect();
	this->addDrawCall(
		backButtonTextBox.getTextureID(),
		backButtonTextBoxRect.getTopLeft(),
		Int2(backButtonTextBoxRect.getWidth(), backButtonTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const UiTextureID highlightTextureID = OptionsUiView::allocHighlightTexture(renderer);
	this->highlightTextureRef.init(highlightTextureID, renderer);
	for (int i = 0; i < OptionsUiModel::OPTION_COUNT; i++)
	{
		UiDrawCall::ActiveFunc highlightActiveFunc = [this, i]()
		{
			const auto &visibleOptions = this->getVisibleOptions();
			if (i >= static_cast<int>(visibleOptions.size()))
			{
				return false;
			}

			auto &game = this->getGame();
			auto &renderer = game.getRenderer();
			const auto &inputManager = game.getInputManager();
			const Int2 mousePosition = inputManager.getMousePosition();
			const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

			DebugAssertIndex(this->optionTextBoxes, i);
			const TextBox &textBox = this->optionTextBoxes[i];
			const Rect &textBoxRect = textBox.getRect();
			return textBoxRect.contains(originalPoint);
		};

		DebugAssertIndex(this->optionTextBoxes, i);
		const TextBox &textBox = this->optionTextBoxes[i];
		const Rect &textBoxRect = textBox.getRect();
		this->addDrawCall(
			[this]() { return this->highlightTextureRef.get(); },
			[textBoxRect]() { return textBoxRect.getTopLeft(); },
			[textBoxRect]() { return Int2(textBoxRect.getWidth(), textBoxRect.getHeight()); },
			[]() { return PivotType::TopLeft; },
			highlightActiveFunc);
	}

	for (TextBox &tabTextBox : this->tabTextBoxes)
	{
		const Rect &tabTextBoxRect = tabTextBox.getRect();
		this->addDrawCall(
			tabTextBox.getTextureID(),
			tabTextBoxRect.getTopLeft(),
			Int2(tabTextBoxRect.getWidth(), tabTextBoxRect.getHeight()),
			PivotType::TopLeft);
	}

	for (int i = 0; i < static_cast<int>(this->optionTextBoxes.size()); i++)
	{
		UiDrawCall::TextureFunc textureFunc = [this, i]()
		{
			DebugAssertIndex(this->optionTextBoxes, i);
			TextBox &optionTextBox = this->optionTextBoxes[i];
			return optionTextBox.getTextureID();
		};

		const TextBox &optionTextBox = this->optionTextBoxes[i];
		const Rect &optionTextBoxRect = optionTextBox.getRect();
		this->addDrawCall(
			textureFunc,
			optionTextBoxRect.getTopLeft(),
			Int2(optionTextBoxRect.getWidth(), optionTextBoxRect.getHeight()),
			PivotType::TopLeft);
	}

	UiDrawCall::TextureFunc descTextureFunc = [this]()
	{
		return this->descriptionTextBox.getTextureID();
	};

	const Rect &descTextBoxRect = this->descriptionTextBox.getRect();
	this->addDrawCall(
		descTextureFunc,
		descTextBoxRect.getTopLeft(),
		Int2(descTextBoxRect.getWidth(), descTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft);

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
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

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
	const std::string text = visibleOption->getName() + ": " + visibleOption->getDisplayedValue();

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
