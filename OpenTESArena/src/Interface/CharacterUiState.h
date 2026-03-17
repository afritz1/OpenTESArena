#ifndef CHARACTER_UI_STATE_H
#define CHARACTER_UI_STATE_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct CharacterUiState
{
	Game *game;
	UiContextInstanceID contextInstID;

	UiTextureID bodyTextureID;
	UiTextureID pantsTextureID;
	UiTextureID headTextureID;
	UiTextureID shirtTextureID;

	CharacterUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace CharacterUI
{
	DECLARE_UI_CONTEXT(Character);

	void onDoneButtonSelected(MouseButtonType mouseButtonType);
	void onNextPageButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(CharacterUI, onDoneButtonSelected),
		DECLARE_UI_FUNC(CharacterUI, onNextPageButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(CharacterUI, onBackInputAction)
	};
}

#endif
