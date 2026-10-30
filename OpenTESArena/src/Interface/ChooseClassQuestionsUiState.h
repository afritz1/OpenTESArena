#pragma once

#include "../Stats/CharacterClassLibrary.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct CharacterClassDefinition;
struct CharacterQuestion;

struct ChooseClassQuestionsUiState
{
	static constexpr int MAX_QUESTIONS = 10;

	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID initialPopUpContextInstID;
	UiContextInstanceID suggestedClassPopUpContextInstID;

	TextureBuilderID bgImageTextureBuilderID;
	PaletteID bgImagePaletteID;
	UiTextureID bgImageTextureID;

	int questionIndices[MAX_QUESTIONS]; // Randomly selected each time entering this UI.
	int questionIndexCount;
	int currentQuestion;
	int categoryAnswerCounts[3]; // # of times class categories have an answer chosen.
	CharacterClassDefinitionID suggestedCharClassDefID;

	double currentScrollAmount;

	CharacterClassCategoryID vfxClassCategoryID;
	double vfxCurrentSeconds;

	ChooseClassQuestionsUiState();

	void init(Game &game);
};

namespace ChooseClassQuestionsUI
{
	DECLARE_UI_CONTEXT(ChooseClassQuestions);

	int getMaxScrollAmount();

	void onAnswerChosen(int optionIndex);
	void onVfxFinished();
	void setQuestionsPresentationVisible(bool visible);
	void setQuestionsTextBoxContent(const CharacterQuestion &question);
	void updateBackgroundTexture();

	void showInitialPopUp();
	void showSuggestedClassPopUp(const CharacterClassDefinition &charClassDef);

	void onMouseButtonHeld(Game &game, MouseButtonType type, Int2 position, double dt);
	void onMouseScrollChanged(Game &game, MouseWheelScrollType type, Int2 position);

	void onScrollDownButtonSelected(MouseButtonType mouseButtonType);
	void onScrollUpButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);
	void onAInputAction(const InputActionCallbackValues &values);
	void onBInputAction(const InputActionCallbackValues &values);
	void onCInputAction(const InputActionCallbackValues &values);

	void onInitialPopUpBackButtonSelected(MouseButtonType mouseButtonType);
	void onInitialPopUpBackInputAction(const InputActionCallbackValues &values);

	void onSuggestedClassPopUpYesButtonSelected(MouseButtonType mouseButtonType);
	void onSuggestedClassPopUpNoButtonSelected(MouseButtonType mouseButtonType);
	void onSuggestedClassPopUpBackInputAction(const InputActionCallbackValues &values);

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
