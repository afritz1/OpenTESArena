#ifndef CHOOSE_GENDER_UI_STATE_H
#define CHOOSE_GENDER_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ChooseGenderUiState
{
	Game *game;
	UiContextState contextState;

	ChooseGenderUiState();

	void init(Game &game);
};

namespace ChooseGenderUI
{
	DECLARE_UI_CONTEXT(ChooseGender);

	void onMaleButtonSelected(MouseButtonType mouseButtonType);
	void onFemaleButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseGenderUI, onMaleButtonSelected),
		DECLARE_UI_FUNC(ChooseGenderUI, onFemaleButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseGenderUI, onBackInputAction)
	};
}

#endif
