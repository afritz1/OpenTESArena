#ifndef LOAD_SAVE_UI_STATE_H
#define LOAD_SAVE_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class LoadSaveType
{
	Load,
	Save
};

struct LoadSaveUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID popUpContextInstID;

	LoadSaveType type;

	LoadSaveUiState();

	void init(Game &game);
};

namespace LoadSaveUI
{
	DECLARE_UI_CONTEXT(LoadSave);

	void onEntrySelected(int index);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		{ "", UiButtonDefinitionCallback() }
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(LoadSaveUI, onBackInputAction)
	};
}

#endif
