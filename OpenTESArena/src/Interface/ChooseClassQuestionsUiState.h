#pragma once

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct CharacterQuestion;

struct ChooseClassQuestionsUiState
{
	static constexpr int MAX_QUESTIONS = 10;

	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID initialPopUpContextInstID;
	UiContextInstanceID decidedClassContextInstID;

	int questionIndices[MAX_QUESTIONS]; // Randomly selected each time.
	int questionIndexCount;
	int currentQuestion;

	double currentScrollAmount;

	ChooseClassQuestionsUiState();

	void init(Game &game);
};

namespace ChooseClassQuestionsUI
{
	DECLARE_UI_CONTEXT(ChooseClassQuestions);

	void onAnswerChosen(int optionIndex);
	void setQuestionsTextBoxContent(const CharacterQuestion &question);

	void showInitialPopUp();
	void showDecidedClassPopUp();

	void onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position);

	void onScrollDownButtonSelected(MouseButtonType mouseButtonType);
	void onScrollUpButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);
	void onAInputAction(const InputActionCallbackValues &values);
	void onBInputAction(const InputActionCallbackValues &values);
	void onCInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassQuestionsUI, onScrollUpButtonSelected),
		DECLARE_UI_FUNC(ChooseClassQuestionsUI, onScrollDownButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ChooseClassQuestionsUI, onBackInputAction),
		DECLARE_UI_FUNC(ChooseClassQuestionsUI, onAInputAction),
		DECLARE_UI_FUNC(ChooseClassQuestionsUI, onBInputAction),
		DECLARE_UI_FUNC(ChooseClassQuestionsUI, onCInputAction)
	};
}
