#pragma once

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct AutomapUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	UiTextureID mapTextureID;
	UiTextureID cursorTextureID;

	Double2 automapOffset;

	AutomapUiState();

	void init(Game &game);
};

// @todo: be able to click somewhere inside the drawable area of the automap and get a 2D voxel
// coordinate in world space for attaching a note to. Store the note in GameState or something.

namespace AutomapUI
{
	DECLARE_UI_CONTEXT(Automap);

	void onMouseButtonHeld(Game &game, MouseButtonType buttonType, const Int2 &position, double dt);

	void onExitButtonSelected(MouseButtonType mouseButtonType);

	void onExitInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(AutomapUI, onExitButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(AutomapUI, onExitInputAction)
	};
}
