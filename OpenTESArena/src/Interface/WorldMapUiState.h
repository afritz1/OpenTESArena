#pragma once

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct WorldMapUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	//UiContextInstanceID fastTravelContextInstID; // Not using a separate context due to update callback limitations

	UiTextureID highlightedProvinceTextureID;
	Buffer<UiTextureID> fastTravelAnimTextureIDs;

	bool isFastTravelling;
	double fastTravelCurrentSeconds;
	double fastTravelTotalSeconds;
	double fastTravelTargetSeconds;
	int fastTravelAnimFrameIndex;

	WorldMapUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
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
