#include "OptionsUiController.h"
#include "OptionsUiState.h"
#include "OptionsUiView.h"
#include "PauseMenuUiState.h"
#include "../Game/Game.h"

namespace
{
	constexpr char ElementName_HighlightImage[] = "OptionsHighlightImage";
	constexpr char ElementName_OptionsListBox[] = "OptionsListBox";
	constexpr char ElementName_DescriptionTextBox[] = "OptionsDescriptionTextBox";

	std::string GetOptionDisplayText(const OptionsUiModel::Option &option)
	{
		char displayText[96];
		std::snprintf(displayText, sizeof(displayText), "%s: %s", option.name.c_str(), option.getDisplayedValue().c_str());
		return std::string(displayText);
	}

	const OptionsUiModel::OptionGroup &GetOptionGroupForTab(OptionsUiModel::Tab tab)
	{
		const OptionsUiState &state = OptionsUI::state;
		switch (tab)
		{
		case OptionsUiModel::Tab::Graphics:
			return state.graphicsOptionGroup;
		case OptionsUiModel::Tab::Audio:
			return state.audioOptionGroup;
		case OptionsUiModel::Tab::Input:
			return state.inputOptionGroup;
		case OptionsUiModel::Tab::Misc:
			return state.miscOptionGroup;
		case OptionsUiModel::Tab::Dev:
			return state.devOptionGroup;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(tab)));
			return state.graphicsOptionGroup;
		}
	}
}

OptionsUiState::OptionsUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->backgroundTextureID = -1;
	this->highlightTextureID = -1;
	this->tab = static_cast<OptionsUiModel::Tab>(-1);
	this->hoveredOptionIndex = -1;
}

void OptionsUiState::init(Game &game)
{
	this->game = &game;

	Renderer &renderer = game.renderer;
	this->backgroundTextureID = OptionsUiView::allocBackgroundTexture(renderer);
	this->highlightTextureID = OptionsUiView::allocHighlightTexture(renderer);

	this->graphicsOptionGroup = OptionsUiModel::makeGraphicsOptionGroup(game);
	this->audioOptionGroup = OptionsUiModel::makeAudioOptionGroup(game);
	this->inputOptionGroup = OptionsUiModel::makeInputOptionGroup(game);
	this->miscOptionGroup = OptionsUiModel::makeMiscOptionGroup(game);
	this->devOptionGroup = OptionsUiModel::makeDevOptionGroup(game);
}

void OptionsUiState::freeTextures(Renderer &renderer)
{
	if (this->backgroundTextureID >= 0)
	{
		renderer.freeUiTexture(this->backgroundTextureID);
		this->backgroundTextureID = -1;
	}

	if (this->highlightTextureID >= 0)
	{
		renderer.freeUiTexture(this->highlightTextureID);
		this->highlightTextureID = -1;
	}
}

void OptionsUI::create(Game &game)
{
	OptionsUiState &state = OptionsUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(OptionsUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo backgroundImageElementInitInfo;
	backgroundImageElementInitInfo.name = "OptionsBackgroundImage";
	backgroundImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backgroundImageElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	backgroundImageElementInitInfo.drawOrder = 0;
	uiManager.createImage(backgroundImageElementInitInfo, state.backgroundTextureID, state.contextInstID, renderer);

	UiElementInitInfo highlightImageElementInitInfo;
	highlightImageElementInitInfo.name = ElementName_HighlightImage;
	highlightImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(highlightImageElementInitInfo, state.highlightTextureID, state.contextInstID, renderer);

	uiManager.addMouseMotionListener(OptionsUI::onMouseMotion, OptionsUI::ContextName, inputManager);
	uiManager.addMouseScrollChangedListener(OptionsUI::onMouseScrollChanged, OptionsUI::ContextName, inputManager);

	const UiElementInstanceID highlightElementInstID = uiManager.getElementByName(ElementName_HighlightImage);
	uiManager.setElementActive(highlightElementInstID, false);

	game.setCursorOverride(std::nullopt);

	// Default to graphics tab.
	OptionsUI::onTabButtonSelected(OptionsUiModel::Tab::Graphics);
}

void OptionsUI::destroy()
{
	OptionsUiState &state = OptionsUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.freeTextures(renderer);
	state.tab = static_cast<OptionsUiModel::Tab>(-1);
	state.hoveredOptionIndex = -1;
}

void OptionsUI::update(double dt)
{
	// Do nothing.
}

void OptionsUI::onMouseMotion(Game &game, int dx, int dy)
{
	OptionsUiState &state = OptionsUI::state;
	UiManager &uiManager = game.uiManager;

	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_OptionsListBox);
	const int itemIndex = uiManager.getListBoxHoveredItemIndex(listBoxElementInstID, game.inputManager, game.window);
	if (itemIndex == state.hoveredOptionIndex)
	{
		return;
	}

	state.hoveredOptionIndex = itemIndex;

	const OptionsUiModel::OptionGroup &optionGroup = GetOptionGroupForTab(state.tab);
	const UiElementInstanceID descriptionTextBoxElementInstID = uiManager.getElementByName(ElementName_DescriptionTextBox);
	const bool isValidItem = itemIndex >= 0;

	std::string descriptionText;
	if (isValidItem)
	{
		const std::unique_ptr<OptionsUiModel::Option> &option = optionGroup[itemIndex];
		descriptionText = option->tooltip;
	}

	uiManager.setTextBoxText(descriptionTextBoxElementInstID, descriptionText.c_str());

	const UiElementInstanceID highlightElementInstID = uiManager.getElementByName(ElementName_HighlightImage);
	if (isValidItem)
	{
		uiManager.setElementActive(highlightElementInstID, true);

		const Rect hoveredItemGlobalRect = uiManager.getListBoxItemGlobalRect(listBoxElementInstID, itemIndex);
		uiManager.setTransformPosition(highlightElementInstID, hoveredItemGlobalRect.getTopLeft());
	}
	else
	{
		uiManager.setElementActive(highlightElementInstID, false);
	}
}

void OptionsUI::onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position)
{
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_OptionsListBox);

	if (type == MouseWheelScrollType::Up)
	{
		uiManager.scrollListBoxUp(listBoxElementInstID);
	}
	else if (type == MouseWheelScrollType::Down)
	{
		uiManager.scrollListBoxDown(listBoxElementInstID);
	}

	OptionsUI::onMouseMotion(game, 0, 0); // Update highlighted item.
}

void OptionsUI::onTabButtonSelected(OptionsUiModel::Tab tab)
{
	OptionsUiState &state = OptionsUI::state;
	if (tab == state.tab)
	{
		return;
	}

	state.tab = tab;

	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_OptionsListBox);
	uiManager.clearListBox(listBoxElementInstID);

	const OptionsUiModel::OptionGroup &optionGroup = GetOptionGroupForTab(tab);
	for (int i = 0; i < static_cast<int>(optionGroup.size()); i++)
	{
		OptionsUiModel::Option &option = *optionGroup[i];

		UiListBoxItem listBoxItem;
		listBoxItem.text = GetOptionDisplayText(option);
		listBoxItem.callback = [&uiManager, listBoxElementInstID, i, &option](MouseButtonType mouseButtonType)
		{
			if (mouseButtonType == MouseButtonType::Left)
			{
				option.tryIncrement();
			}
			else if (mouseButtonType == MouseButtonType::Right)
			{
				option.tryDecrement();
			}

			const std::string newOptionDisplayText = GetOptionDisplayText(option);
			uiManager.setListBoxItemText(listBoxElementInstID, i, newOptionDisplayText.c_str());
		};

		uiManager.insertBackListBoxItem(listBoxElementInstID, std::move(listBoxItem));
	}
}

void OptionsUI::onGraphicsButtonSelected(MouseButtonType mouseButtonType)
{
	OptionsUI::onTabButtonSelected(OptionsUiModel::Tab::Graphics);
}

void OptionsUI::onAudioButtonSelected(MouseButtonType mouseButtonType)
{
	OptionsUI::onTabButtonSelected(OptionsUiModel::Tab::Audio);
}

void OptionsUI::onInputButtonSelected(MouseButtonType mouseButtonType)
{
	OptionsUI::onTabButtonSelected(OptionsUiModel::Tab::Input);
}

void OptionsUI::onMiscButtonSelected(MouseButtonType mouseButtonType)
{
	OptionsUI::onTabButtonSelected(OptionsUiModel::Tab::Misc); 
}

void OptionsUI::onDevButtonSelected(MouseButtonType mouseButtonType)
{
	OptionsUI::onTabButtonSelected(OptionsUiModel::Tab::Dev);
}

void OptionsUI::onBackButtonSelected(MouseButtonType mouseButtonType)
{
	OptionsUiState &state = OptionsUI::state;
	Game &game = *state.game;
	game.setNextContext(PauseMenuUI::ContextName);
}

void OptionsUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		OptionsUI::onBackButtonSelected(MouseButtonType::Left);
	}
}
