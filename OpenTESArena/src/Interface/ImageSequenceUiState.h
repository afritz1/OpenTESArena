#ifndef IMAGE_SEQUENCE_UI_STATE_H
#define IMAGE_SEQUENCE_UI_STATE_H

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

using ImageSequenceFinishedCallback = std::function<void()>;

// Must be set before UI context is started.
struct ImageSequenceUiInitInfo
{
	Buffer<std::string> paletteNames;
	Buffer<std::string> textureNames;
	Buffer<double> imageDurations;
	ImageSequenceFinishedCallback callback;

	ImageSequenceUiInitInfo();

	void init(Span<const std::string> paletteNames, Span<const std::string> textureNames, Span<const double> imageDurations, const ImageSequenceFinishedCallback &callback);
};

struct ImageSequenceUiState
{
	ImageSequenceUiInitInfo initInfo;

	Game *game;
	UiContextInstanceID contextInstID;

	Buffer<UiTextureID> textureIDs;
	Buffer<double> imageDurations;
	double currentSeconds;
	int imageIndex;
	ImageSequenceFinishedCallback callback;

	ImageSequenceUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace ImageSequenceUI
{
	DECLARE_UI_CONTEXT(ImageSequence);

	// @improvement this could be a mouse button change handler instead so the invisible mouse skip never fails even outside the 320x200 range
	void onSkipButtonSelected(MouseButtonType mouseButtonType);

	void onSkipInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(ImageSequenceUI, onSkipButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(ImageSequenceUI, onSkipInputAction)
	};
}

#endif
