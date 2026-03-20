#ifndef TEXT_CINEMATIC_UI_STATE_H
#define TEXT_CINEMATIC_UI_STATE_H

#include "TextCinematicUiModel.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"
#include "../UI/UiLibrary.h"

class Game;

enum class MouseButtonType;

using TextCinematicFinishedCallback = std::function<void()>;

// Must be set before UI context is started.
struct TextCinematicUiInitInfo
{
	int textCinematicDefIndex;
	double secondsPerImage;
	TextCinematicFinishedCallback callback;

	TextCinematicUiInitInfo();

	void init(int textCinematicDefIndex, double secondsPerImage, const TextCinematicFinishedCallback &callback);
};

struct TextCinematicUiState
{
	TextCinematicUiInitInfo initInfo;

	Game *game;
	UiContextInstanceID contextInstID;

	Buffer<UiTextureID> animTextureIDs; // One per animation frame.
	std::vector<std::string> textPages; // One string per page of text.
	TextCinematicSpeechState speechState;
	double secondsPerImage, currentImageSeconds;
	int animImageIndex, textIndex, textCinematicDefIndex;
	TextCinematicFinishedCallback callback;

	TextCinematicUiState();

	void init(Game &game);
	void freeTextures(Renderer &renderer);
};

namespace TextCinematicUI
{
	DECLARE_UI_CONTEXT(TextCinematic);

	void updateSubtitles();

	// @improvement this could be a mouse button change handler instead so the invisible mouse skip never fails even outside the 320x200 range
	void onSkipButtonSelected(MouseButtonType mouseButtonType);

	void onSkipInputAction(const InputActionCallbackValues &values);

	constexpr std::pair<const char*, UiButtonDefinitionCallback> ButtonCallbacks[] =
	{
		DECLARE_UI_FUNC(TextCinematicUI, onSkipButtonSelected)
	};

	constexpr std::pair<const char*, UiInputListenerDefinitionCallback> InputActionCallbacks[] =
	{
		DECLARE_UI_FUNC(TextCinematicUI, onSkipInputAction)
	};
}

#endif
