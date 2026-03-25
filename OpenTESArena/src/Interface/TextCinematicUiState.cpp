#include <algorithm>

#include "CinematicLibrary.h"
#include "TextCinematicUiState.h"
#include "TextCinematicUiView.h"
#include "../Game/Game.h"

namespace
{
	constexpr char ElementName_TextCinematicImage[] = "TextCinematicImage";
	constexpr char ElementName_TextCinematicSubtitlesTextBox[] = "TextCinematicSubtitlesTextBox";
}

TextCinematicUiInitInfo::TextCinematicUiInitInfo()
{
	this->textCinematicDefIndex = -1;
	this->secondsPerImage = 0.0;
	this->callback = []() { };
}

void TextCinematicUiInitInfo::init(int textCinematicDefIndex, double secondsPerImage, const TextCinematicFinishedCallback &callback)
{
	DebugAssert(secondsPerImage > 0.0);
	this->textCinematicDefIndex = textCinematicDefIndex;
	this->secondsPerImage = secondsPerImage;
	this->callback = callback;
}

TextCinematicUiState::TextCinematicUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->secondsPerImage = 0.0;
	this->currentImageSeconds = 0.0;
	this->animImageIndex = -1;
	this->textIndex = -1;
	this->textCinematicDefIndex = -1;
	this->callback = []() { };
}

void TextCinematicUiState::init(Game &game)
{
	DebugAssert(this->initInfo.secondsPerImage > 0.0);
	this->game = &game;

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	
	const CinematicLibrary &cinematicLibrary = CinematicLibrary::getInstance();
	const TextCinematicDefinition &textCinematicDef = cinematicLibrary.getTextDefinition(this->initInfo.textCinematicDefIndex);
	this->animTextureIDs = TextCinematicUiView::allocAnimationTextures(textCinematicDef.animFilename, textureManager, renderer);

	const std::string subtitleText = TextCinematicUiModel::getSubtitleText(game, textCinematicDef);
	this->textPages = TextCinematicUiModel::getSubtitleTextPages(subtitleText);

	if (TextCinematicUiModel::shouldPlaySpeech(game))
	{
		this->speechState.init(textCinematicDef.templateDatKey);
	}

	this->secondsPerImage = this->initInfo.secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->animImageIndex = 0;
	this->textIndex = 0;
	this->textCinematicDefIndex = this->initInfo.textCinematicDefIndex;
	this->callback = this->initInfo.callback;
}

void TextCinematicUiState::freeTextures(Renderer &renderer)
{
	if (this->animTextureIDs.isValid())
	{
		for (UiTextureID textureID : this->animTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->animTextureIDs.clear();
	}
}

void TextCinematicUI::create(Game &game)
{
	TextCinematicUiState &state = TextCinematicUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(TextCinematicUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo imageElementInitInfo;
	imageElementInitInfo.name = ElementName_TextCinematicImage;
	uiManager.createImage(imageElementInitInfo, state.animTextureIDs[0], state.contextInstID, renderer);

	const CinematicLibrary &cinematicLibrary = CinematicLibrary::getInstance();
	const TextCinematicDefinition &textCinematicDef = cinematicLibrary.getTextDefinition(state.textCinematicDefIndex);
	const UiElementInstanceID subtitlesTextBoxElementInstID = uiManager.getElementByName(ElementName_TextCinematicSubtitlesTextBox);
	uiManager.setTextBoxColor(subtitlesTextBoxElementInstID, textCinematicDef.fontColor);

	TextCinematicUI::updateSubtitles();

	game.setCursorOverride(std::nullopt);
	uiManager.setElementActive(game.cursorImageElementInstID, false);
}

void TextCinematicUI::destroy()
{
	TextCinematicUiState &state = TextCinematicUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.freeTextures(renderer);
	state.secondsPerImage = 0.0;
	state.currentImageSeconds = 0.0;
	state.animImageIndex = -1;
	state.textIndex = -1;
	state.textCinematicDefIndex = -1;
	state.callback = []() { };

	uiManager.setElementActive(game.cursorImageElementInstID, true);
	
	// Stop voice if still playing.
	AudioManager &audioManager = game.audioManager;
	audioManager.stopSound();
}

void TextCinematicUI::update(double dt)
{
	TextCinematicUiState &state = TextCinematicUI::state;
	Game &game = *state.game;

	const UiTextureID prevTextureID = state.animTextureIDs[state.animImageIndex];

	// Animation.
	state.currentImageSeconds += dt;
	while (state.currentImageSeconds > state.secondsPerImage)
	{
		state.currentImageSeconds -= state.secondsPerImage;
		state.animImageIndex++;

		if (state.animImageIndex == state.animTextureIDs.getCount())
		{
			state.animImageIndex = 0;
		}
	}

	const UiTextureID currentTextureID = state.animTextureIDs[state.animImageIndex];
	if (currentTextureID != prevTextureID)
	{
		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID imageElementInstID = uiManager.getElementByName(ElementName_TextCinematicImage);
		uiManager.setImageTexture(imageElementInstID, currentTextureID);
	}

	if (TextCinematicUiModel::shouldPlaySpeech(game))
	{
		// Update speech state, optionally ending the cinematic if done with last speech.
		AudioManager &audioManager = game.audioManager;

		const bool playedFirstVoice = !TextCinematicSpeechState::isFirstVoice(state.speechState.getNextVoiceIndex());
		if (!playedFirstVoice)
		{
			const std::string voiceFilename = state.speechState.getVoiceFilename(state.speechState.getNextVoiceIndex());
			audioManager.playSound(voiceFilename.c_str());
			state.speechState.incrementVoiceIndex();
		}
		else
		{
			const int prevVoiceIndex = state.speechState.getNextVoiceIndex() - 1;
			const std::string prevVoiceFilename = state.speechState.getVoiceFilename(prevVoiceIndex);

			// Wait until previous voice is done playing.
			if (!audioManager.isPlayingSound(prevVoiceFilename))
			{
				const int nextVoiceIndex = state.speechState.getNextVoiceIndex();
				const std::string nextVoiceFilename = state.speechState.getVoiceFilename(nextVoiceIndex);

				if (audioManager.soundExists(nextVoiceFilename))
				{
					audioManager.playSound(nextVoiceFilename.c_str());
					state.speechState.incrementVoiceIndex();

					if (TextCinematicSpeechState::isBeginningOfNewPage(nextVoiceIndex))
					{
						state.textIndex++;
						TextCinematicUI::updateSubtitles();
					}
				}
				else
				{
					// No remaining voices to play.
					state.callback();
				}
			}
		}
	}
}

void TextCinematicUI::updateSubtitles()
{
	TextCinematicUiState &state = TextCinematicUI::state;
	Game &game = *state.game;
	
	std::string textToDisplay;
	if (state.textIndex < static_cast<int>(state.textPages.size()))
	{
		textToDisplay = state.textPages[state.textIndex];
	}

	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(ElementName_TextCinematicSubtitlesTextBox);
	uiManager.setTextBoxText(textBoxElementInstID, textToDisplay.c_str());
}

void TextCinematicUI::onSkipButtonSelected(MouseButtonType mouseButtonType)
{
	TextCinematicUiState &state = TextCinematicUI::state;
	Game &game = *state.game;

	if (!TextCinematicUiModel::shouldPlaySpeech(game))
	{
		state.textIndex++;

		const int textBoxCount = static_cast<int>(state.textPages.size());
		if (state.textIndex >= textBoxCount)
		{
			state.textIndex = textBoxCount - 1;
			state.callback();
		}

		TextCinematicUI::updateSubtitles();
	}
}

void TextCinematicUI::onSkipInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		TextCinematicUiState &state = TextCinematicUI::state;
		state.callback();
	}
}
