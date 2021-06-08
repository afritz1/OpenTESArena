#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "SDL.h"

#include "TextCinematicPanel.h"
#include "TextCinematicUiView.h"
#include "../Audio/AudioManager.h"
#include "../Game/Game.h"
#include "../Math/Vector2.h"
#include "../Media/TextCinematicDefinition.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

#include "components/debug/Debug.h"

TextCinematicPanel::TextCinematicPanel(Game &game)
	: Panel(game) { }

bool TextCinematicPanel::init(int textCinematicDefIndex, double secondsPerImage,
	const std::function<void(Game&)> &endingAction)
{
	auto &game = this->getGame();
	const auto &cinematicLibrary = game.getCinematicLibrary();
	const TextCinematicDefinition &textCinematicDef = cinematicLibrary.getTextDefinition(textCinematicDefIndex);

	this->textBoxes = [&game, &textCinematicDef]()
	{
		const std::string subtitleText = TextCinematicUiModel::getSubtitleText(game, textCinematicDef);
		const int subtitleTextLineCount = TextCinematicUiModel::getSubtitleTextLineCount(subtitleText);
		const int textBoxesToMake = TextCinematicUiModel::getSubtitleTextBoxCount(subtitleTextLineCount);
		const std::vector<std::string> subtitleTextLines = TextCinematicUiModel::getSubtitleTextLines(subtitleText);

		// Group up to three text lines per text box.
		std::vector<std::unique_ptr<TextBox>> textBoxes;
		for (int i = 0; i < textBoxesToMake; i++)
		{
			std::string textBoxText;
			int linesToUse = std::min(subtitleTextLineCount - (i * 3), 3);
			for (int j = 0; j < linesToUse; j++)
			{
				const int textLineIndex = j + (i * 3);
				DebugAssertIndex(subtitleTextLines, textLineIndex);
				const std::string &textLine = subtitleTextLines[textLineIndex];
				textBoxText.append(textLine);
				textBoxText.append("\n");
			}

			// Avoid adding empty text boxes.
			if (textBoxText.size() == 0)
			{
				continue;
			}

			// Eventually use a different color when other cinematic actors are supported.
			const auto &fontLibrary = game.getFontLibrary();
			const RichTextString richText(
				textBoxText,
				TextCinematicUiView::SubtitleTextBoxFontName,
				textCinematicDef.getFontColor(),
				TextCinematicUiView::SubtitleTextBoxTextAlignment,
				TextCinematicUiView::SubtitleTextBoxLineSpacing,
				fontLibrary);

			auto textBox = std::make_unique<TextBox>(
				TextCinematicUiView::SubtitleTextBoxCenterPoint,
				richText,
				fontLibrary,
				game.getRenderer());

			textBoxes.emplace_back(std::move(textBox));
		}

		return textBoxes;
	}();

	this->skipButton = Button<Game&>(endingAction);

	// Optionally initialize speech state if speech is available.
	if (TextCinematicUiModel::shouldPlaySpeech(game))
	{
		this->speechState.init(textCinematicDef.getTemplateDatKey());
	}

	this->animTextureFilename = textCinematicDef.getAnimationFilename();
	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->textCinematicDefIndex = textCinematicDefIndex;
	this->animImageIndex = 0;
	this->textIndex = 0;

	return true;
}

void TextCinematicPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		// Force the cinematic to end.
		this->skipButton.click(game);

		// Stop voice if it is still playing.
		auto &audioManager = game.getAudioManager();
		audioManager.stopSound();
	}

	// Only allow page skipping if there is no speech.
	if (!TextCinematicUiModel::shouldPlaySpeech(game))
	{
		const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
		const bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);
		const bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) || inputManager.keyPressed(e, SDLK_KP_ENTER);
		const bool skipHotkeyPressed = spacePressed || enterPressed;

		if (leftClick || skipHotkeyPressed)
		{
			this->textIndex++;

			// If done with the last text box, then prepare for the next panel.
			int textBoxCount = static_cast<int>(this->textBoxes.size());
			if (this->textIndex >= textBoxCount)
			{
				this->textIndex = textBoxCount - 1;
				this->skipButton.click(game);
			}
		}
	}
}

void TextCinematicPanel::tick(double dt)
{
	this->currentImageSeconds += dt;

	while (this->currentImageSeconds > this->secondsPerImage)
	{
		this->currentImageSeconds -= this->secondsPerImage;
		this->animImageIndex++;

		auto &textureManager = this->getGame().getTextureManager();
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(this->animTextureFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugCrash("Couldn't get anim texture file metadata for \"" + this->animTextureFilename + "\".");
		}

		// If at the end of the sequence, go back to the first image. The cinematic ends at the end
		// of the last text box.
		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
		if (this->animImageIndex == textureFileMetadata.getTextureCount())
		{
			this->animImageIndex = 0;
		}
	}

	auto &game = this->getGame();
	if (TextCinematicUiModel::shouldPlaySpeech(game))
	{
		// Update speech state, optionally ending the cinematic if done with last speech.
		auto &audioManager = game.getAudioManager();

		const bool playedFirstVoice = !TextCinematicUiModel::SpeechState::isFirstVoice(this->speechState.getNextVoiceIndex());
		if (!playedFirstVoice)
		{
			const std::string voiceFilename = this->speechState.getVoiceFilename(this->speechState.getNextVoiceIndex());
			audioManager.playSound(voiceFilename);
			this->speechState.incrementVoiceIndex();
		}
		else
		{
			const int prevVoiceIndex = this->speechState.getNextVoiceIndex() - 1;
			const std::string prevVoiceFilename = this->speechState.getVoiceFilename(prevVoiceIndex);

			// Wait until previous voice is done playing.
			if (!audioManager.isPlayingSound(prevVoiceFilename))
			{
				const int nextVoiceIndex = this->speechState.getNextVoiceIndex();
				const std::string nextVoiceFilename = this->speechState.getVoiceFilename(nextVoiceIndex);

				if (audioManager.soundExists(nextVoiceFilename))
				{
					audioManager.playSound(nextVoiceFilename);
					this->speechState.incrementVoiceIndex();

					if (TextCinematicUiModel::SpeechState::isBeginningOfNewPage(nextVoiceIndex))
					{
						this->textIndex++;
					}
				}
				else
				{
					// No remaining voices to play.
					this->skipButton.click(game);
				}
			}
		}
	}
}

void TextCinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw current frame in animation.
	auto &textureManager = this->getGame().getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(this->animTextureFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + this->animTextureFilename + "\".");
		return;
	}

	const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(this->animTextureFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugLogError("Couldn't get texture builder IDs for \"" + this->animTextureFilename + "\".");
		return;
	}

	const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(this->animImageIndex);
	renderer.drawOriginal(textureBuilderID, *paletteID, textureManager);

	// Draw the relevant text box.
	DebugAssertIndex(this->textBoxes, this->textIndex);
	const auto &textBox = this->textBoxes[this->textIndex];
	renderer.drawOriginal(textBox->getTexture(), textBox->getX(), textBox->getY());
}
