#ifndef CINEMATIC_UI_STATE_H
#define CINEMATIC_UI_STATE_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

using CinematicFinishedCallback = std::function<void()>;

// Must be set before UI context is started.
struct CinematicUiInitInfo
{
	std::string paletteName;
	std::string sequenceName;
	double secondsPerImage;
	CinematicFinishedCallback callback;

	CinematicUiInitInfo();

	void init(const std::string &paletteName, const std::string &sequenceName, double secondsPerImage, const CinematicFinishedCallback &callback);
};

struct CinematicUiState
{
	CinematicUiInitInfo initInfo;

	Game *game;
	UiContextInstanceID contextInstID;

	Buffer<UiTextureID> videoTextureIDs;
	double secondsPerImage, currentSeconds;
	int imageIndex;
	CinematicFinishedCallback callback;

	CinematicUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace CinematicUI
{
	DECLARE_UI_CONTEXT(Cinematic);

	// @improvement this could be a mouse button change handler instead so the invisible mouse skip never fails even outside the 320x200 range
	void onSkipButtonSelected(MouseButtonType mouseButtonType);

	void onSkipInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(CinematicUI, onSkipButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(CinematicUI, onSkipInputAction)
	};
}

#endif
