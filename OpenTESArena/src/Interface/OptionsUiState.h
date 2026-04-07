#pragma once

#include "OptionsUiMVC.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

struct OptionsUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	UiTextureID backgroundTextureID;
	UiTextureID highlightTextureID; // When hovering an option.

	OptionsUiModel::OptionGroup graphicsOptionGroup;
	OptionsUiModel::OptionGroup audioOptionGroup;
	OptionsUiModel::OptionGroup inputOptionGroup;
	OptionsUiModel::OptionGroup miscOptionGroup;
	OptionsUiModel::OptionGroup devOptionGroup;
	OptionsUiModel::Tab tab;
	int hoveredOptionIndex;

	OptionsUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace OptionsUI
{
	DECLARE_UI_CONTEXT(Options);

	void onMouseMotion(Game &game, int dx, int dy);
	void onMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position);

	void onTabButtonSelected(OptionsUiModel::Tab tab);
	void onGraphicsButtonSelected(MouseButtonType mouseButtonType);
	void onAudioButtonSelected(MouseButtonType mouseButtonType);
	void onInputButtonSelected(MouseButtonType mouseButtonType);
	void onMiscButtonSelected(MouseButtonType mouseButtonType);
	void onDevButtonSelected(MouseButtonType mouseButtonType);
	void onBackButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(OptionsUI, onGraphicsButtonSelected),
		DECLARE_UI_FUNC(OptionsUI, onAudioButtonSelected),
		DECLARE_UI_FUNC(OptionsUI, onInputButtonSelected),
		DECLARE_UI_FUNC(OptionsUI, onMiscButtonSelected),
		DECLARE_UI_FUNC(OptionsUI, onDevButtonSelected),
		DECLARE_UI_FUNC(OptionsUI, onBackButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(OptionsUI, onBackInputAction)
	};
}
