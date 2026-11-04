#pragma once

#include <unordered_map>

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct AutomapNoteEntry
{
	static constexpr int MAX_CHARACTERS = 64;

	Int2 position;
	std::string text;
	bool inGameWorld;

	AutomapNoteEntry();
};

struct AutomapUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	UiTextureID mapTextureID;
	UiTextureID cursorTextureID;

	Double2 automapOffset;

	UiElementInstanceID currentNoteElementInstID;
	std::string currentNote;
	bool isCurrentNoteInGameWorld;
	std::unordered_map<UiElementInstanceID, int> gameWorldNotes; // Notes that move with the automap.

	AutomapUiState();

	void init(Game &game);
};

namespace AutomapUI
{
	DECLARE_UI_CONTEXT(Automap);

	void beginNote(Int2 originalPosition);
	void confirmNote();
	void cancelNote();

	void onMouseButtonChanged(Game &game, MouseButtonType type, Int2 position, bool pressed);
	void onMouseButtonHeld(Game &game, MouseButtonType buttonType, Int2 position, double dt);
	void onTextInput(const std::string_view text);

	void onRecenterButtonSelected(MouseButtonType mouseButtonType);
	void onExitButtonSelected(MouseButtonType mouseButtonType);

	void onAcceptInputAction(const InputActionCallbackValues &values);
	void onBackInputAction(const InputActionCallbackValues &values);
	void onBackspaceInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(AutomapUI, onRecenterButtonSelected),
		DECLARE_UI_FUNC(AutomapUI, onExitButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(AutomapUI, onAcceptInputAction),
		DECLARE_UI_FUNC(AutomapUI, onBackInputAction),
		DECLARE_UI_FUNC(AutomapUI, onBackspaceInputAction)
	};
}
