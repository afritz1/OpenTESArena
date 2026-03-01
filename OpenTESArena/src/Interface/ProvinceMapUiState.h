#ifndef PROVINCE_MAP_UI_STATE_H
#define PROVINCE_MAP_UI_STATE_H

#include "../UI/AnimationState.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

struct ProvinceMapLocationTextures
{
	UiTextureID textureID, playerCurrentTextureID, travelDestinationTextureID;

	ProvinceMapLocationTextures();

	void init(UiTextureID textureID, UiTextureID playerCurrentTextureID, UiTextureID travelDestinationTextureID, Renderer &renderer);
	void free(Renderer &renderer);
};

struct ProvinceMapUiState
{
	Game *game;
	UiContextInstanceID contextInstID;
	UiContextInstanceID textPopUpContextInstID;
	UiContextInstanceID searchPopUpContextInstID;

	UiTextureID backgroundTextureID;

	ProvinceMapLocationTextures cityStateTextures, townTextures, villageTextures, dungeonTextures, staffDungeonTextures;
	AnimationState blinkState;
	int provinceID;
	int hoveredLocationID;

	ProvinceMapUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace ProvinceMapUI
{
	DECLARE_UI_CONTEXT(ProvinceMap);

	void initLocationIconUI(int provinceID);
	void updateHoveredLocationID(Int2 originalPosition);
	void trySelectLocation(int selectedLocationID);
	void beginFastTravel();
	void showTextPopUp(const char *str);
	void onPauseChanged(bool paused);

	// @todo makeDiseasedWarningPopUp() when the player is diseased

	void onMouseMotion(Game &game, int dx, int dy);

	void onFullscreenButtonSelected(MouseButtonType mouseButtonType);

	void onBackInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ProvinceMapUI, onFullscreenButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ProvinceMapUI, onBackInputAction)
	};
}

#endif
