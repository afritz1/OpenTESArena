#pragma once

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

struct PauseMenuUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	UiTextureID statusBarsTextureID; // Health + stamina + spell points.
	UiTextureID statusGradientTextureID;
	UiTextureID playerPortraitTextureID;
	UiTextureID optionsButtonTextureID;

	PauseMenuUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace PauseMenuUI
{
	DECLARE_UI_CONTEXT(PauseMenu);

	void setSoundText(double volumePercent);
	void setMusicText(double volumePercent);

	void onNewGameButtonSelected(MouseButtonType mouseButtonType);
	void onLoadGameButtonSelected(MouseButtonType mouseButtonType);
	void onSaveGameButtonSelected(MouseButtonType mouseButtonType);
	void onExitGameButtonSelected(MouseButtonType mouseButtonType);
	void onResumeGameButtonSelected(MouseButtonType mouseButtonType);
	void onOptionsButtonSelected(MouseButtonType mouseButtonType);
	void onSoundUpButtonSelected(MouseButtonType mouseButtonType);
	void onSoundDownButtonSelected(MouseButtonType mouseButtonType);
	void onMusicUpButtonSelected(MouseButtonType mouseButtonType);
	void onMusicDownButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(PauseMenuUI, onNewGameButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onLoadGameButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onSaveGameButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onExitGameButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onResumeGameButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onOptionsButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onSoundUpButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onSoundDownButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onMusicUpButtonSelected),
		DECLARE_UI_FUNC(PauseMenuUI, onMusicDownButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(PauseMenuUI, onBackInputAction)
	};
}
