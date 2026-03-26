#ifndef CHOOSE_CLASS_UI_STATE_H
#define CHOOSE_CLASS_UI_STATE_H

#include <vector>

#include "../Stats/CharacterClassDefinition.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;
enum class MouseWheelScrollType;

struct ChooseClassUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	std::vector<CharacterClassDefinition> charClasses;
	int hoveredListBoxItemIndex;

	ChooseClassUiState();

	void init(Game &game);
};

namespace ChooseClassUI
{
	DECLARE_UI_CONTEXT(ChooseClass);

	void onListBoxItemSelected(int index);
	void updateListBoxHoveredIndex();

	void onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position);
	void onMouseMotion(Game &game, int dx, int dy);

	void onScrollUpButtonSelected(MouseButtonType mouseButtonType);
	void onScrollDownButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassUI, onScrollUpButtonSelected),
		DECLARE_UI_FUNC(ChooseClassUI, onScrollDownButtonSelected),
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassUI, onBackInputAction)
	};
}

#endif
