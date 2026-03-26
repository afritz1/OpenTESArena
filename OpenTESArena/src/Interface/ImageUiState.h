#ifndef IMAGE_UI_STATE_H
#define IMAGE_UI_STATE_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

using ImageFinishedCallback = std::function<void()>;

// Must be set before UI context is started.
struct ImageUiInitInfo
{
	std::string paletteName;
	std::string textureName;
	double secondsToDisplay;
	ImageFinishedCallback callback;

	ImageUiInitInfo();

	void init(const std::string &paletteName, const std::string &textureName, double secondsToDisplay, const ImageFinishedCallback &callback);
};

struct ImageUiState
{
	ImageUiInitInfo initInfo;

	Game *game;
	UiContextInstanceID contextInstID;

	double secondsToDisplay, currentSeconds;
	ImageFinishedCallback callback;

	ImageUiState();

	void init(Game &game);
};

namespace ImageUI
{
	DECLARE_UI_CONTEXT(Image);

	// @improvement this could be a mouse button change handler instead so the invisible mouse skip never fails even outside the 320x200 range
	void onSkipButtonSelected(MouseButtonType mouseButtonType);

	void onSkipInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ImageUI, onSkipButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ImageUI, onSkipInputAction)
	};
}

#endif
