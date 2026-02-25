#ifndef WORLD_MAP_UI_STATE_H
#define WORLD_MAP_UI_STATE_H

#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct WorldMapUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	// @todo fast travel animation context

	UiTextureID highlightedProvinceTextureID;

	WorldMapUiState();

	void init(Game &game);
};

namespace WorldMapUI
{
	DECLARE_UI_CONTEXT(WorldMap);

	void onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed);

	void onBackButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(WorldMapUI, onBackButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(WorldMapUI, onBackInputAction)
	};
}

#endif
