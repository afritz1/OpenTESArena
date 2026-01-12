#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "ChooseClassUiState.h"
#include "../Game/Game.h"
#include "../Stats/CharacterClassLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

namespace
{
	constexpr char ListBoxElementName[] = "ChooseClassListBox";
	constexpr char ClassDescriptionElementName[] = "ChooseClassDescriptionTextBox";
}

ChooseClassUiState::ChooseClassUiState()
{
	this->game = nullptr;
	this->hoveredListBoxItemIndex = -1;
}

void ChooseClassUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseClassUI::create(Game &game)
{
	ChooseClassUiState &state = ChooseClassUI::state;
	state.init(game);

	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	UiManager &uiManager = game.uiManager;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseClassUI::ContextType);
	uiManager.createContext(contextDef, state.contextState, inputManager, textureManager, renderer);

	const std::string titleText = ChooseClassUiModel::getTitleText(game);
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.getElementByName("ChooseClassTitleTextBox");
	uiManager.setTextBoxText(titleTextBoxElementInstID, titleText.c_str());

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	state.charClasses = std::vector<CharacterClassDefinition>(charClassLibrary.getDefinitionCount());
	for (int i = 0; i < static_cast<int>(state.charClasses.size()); i++)
	{
		state.charClasses[i] = charClassLibrary.getDefinition(i);
	}

	std::sort(state.charClasses.begin(), state.charClasses.end(),
		[](const CharacterClassDefinition &a, const CharacterClassDefinition &b)
	{
		return StringView::compare(a.name, b.name) < 0;
	});

	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ListBoxElementName);
	for (int i = 0; i < static_cast<int>(state.charClasses.size()); i++)
	{
		const CharacterClassDefinition &charClassDef = state.charClasses[i];

		UiListBoxItem listBoxItem;
		listBoxItem.text = charClassDef.name;
		listBoxItem.callback = [i]() { ChooseClassUI::onListBoxItemSelected(i); };
		uiManager.insertBackListBoxItem(listBoxElementInstID, std::move(listBoxItem));
	}

	const InputListenerID mouseScrollChangedListenerID = inputManager.addMouseScrollChangedListener(ChooseClassUI::onMouseScrollChanged);
	state.contextState.mouseScrollChangedListenerIDs.emplace_back(mouseScrollChangedListenerID);

	const InputListenerID mouseMotionListenerID = inputManager.addMouseMotionListener(ChooseClassUI::onMouseMotion);
	state.contextState.mouseMotionListenerIDs.emplace_back(mouseMotionListenerID);

	ChooseClassUI::updateListBoxHoveredIndex();
}

void ChooseClassUI::destroy()
{
	ChooseClassUiState &state = ChooseClassUI::state;
	Game &game = *state.game;
	state.contextState.free(game.inputManager, game.uiManager, game.renderer);
	state.charClasses.clear();
	state.hoveredListBoxItemIndex = -1;
}

void ChooseClassUI::update(double dt)
{
	ChooseClassUiState &state = ChooseClassUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	UiElementInstanceID descriptionTextBoxElementInstID = uiManager.getElementByName(ClassDescriptionElementName);

	const bool isHoveredClassValid = state.hoveredListBoxItemIndex >= 0;
	uiManager.setElementActive(descriptionTextBoxElementInstID, isHoveredClassValid);
}

void ChooseClassUI::onListBoxItemSelected(int index)
{
	ChooseClassUiState &state = ChooseClassUI::state;
	Game &game = *state.game;
	const CharacterClassDefinition &charClassDef = state.charClasses[index];

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	int charClassDefID;
	if (!charClassLibrary.tryGetDefinitionIndex(charClassDef, &charClassDefID))
	{
		DebugLogErrorFormat("Couldn't get index of character class definition \"%s\".", charClassDef.name);
		return;
	}

	ChooseClassUiController::onItemButtonSelected(game, charClassDefID);
}

void ChooseClassUI::updateListBoxHoveredIndex()
{
	ChooseClassUiState &state = ChooseClassUI::state;
	Game &game = *state.game;

	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ListBoxElementName);
	const int hoveredItemIndex = uiManager.getListBoxHoveredItemIndex(listBoxElementInstID, game.inputManager, game.window);
	if (hoveredItemIndex != state.hoveredListBoxItemIndex)
	{
		state.hoveredListBoxItemIndex = hoveredItemIndex;

		if (hoveredItemIndex >= 0)
		{
			DebugAssertIndex(state.charClasses, hoveredItemIndex);
			const CharacterClassDefinition &charClassDef = state.charClasses[hoveredItemIndex];
			const std::string text = ChooseClassUiModel::getFullTooltipText(charClassDef, game);
			const UiElementInstanceID descriptionTextBoxElementInstID = uiManager.getElementByName(ClassDescriptionElementName);
			uiManager.setTextBoxText(descriptionTextBoxElementInstID, text.c_str());
		}
	}
}

void ChooseClassUI::onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position)
{
	const UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ListBoxElementName);
	const Rect listBoxRect = uiManager.getTransformGlobalRect(listBoxElementInstID);
	const Int2 classicMousePosition = game.window.nativeToOriginal(position);

	if (listBoxRect.contains(classicMousePosition))
	{
		bool isHoveredIndexDirty = false;

		if (type == MouseWheelScrollType::Down)
		{
			ChooseClassUI::onScrollDownButtonSelected(MouseButtonType::Left);
			isHoveredIndexDirty = true;
		}
		else if (type == MouseWheelScrollType::Up)
		{
			ChooseClassUI::onScrollUpButtonSelected(MouseButtonType::Left);
			isHoveredIndexDirty = true;
		}

		if (isHoveredIndexDirty)
		{
			ChooseClassUI::updateListBoxHoveredIndex();
		}
	}
}

void ChooseClassUI::onMouseMotion(Game &game, int dx, int dy)
{
	ChooseClassUI::updateListBoxHoveredIndex();
}

void ChooseClassUI::onScrollUpButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassUiState &state = ChooseClassUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ListBoxElementName);
	uiManager.scrollListBoxUp(listBoxElementInstID);
}

void ChooseClassUI::onScrollDownButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassUiState &state = ChooseClassUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ListBoxElementName);
	uiManager.scrollListBoxDown(listBoxElementInstID);
}

void ChooseClassUI::onBackInputAction(const InputActionCallbackValues &values)
{
	ChooseClassUiController::onBackToChooseClassCreationInputAction(values);
}
