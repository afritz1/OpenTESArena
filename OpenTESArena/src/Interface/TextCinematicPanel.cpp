#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "SDL.h"

#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
#include "Texture.h"
#include "../Game/Game.h"
#include "../Math/Vector2.h"
#include "../Media/AudioManager.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/TextCinematicDefinition.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"
#include "components/dos/DOSUtils.h"
#include "components/utilities/String.h"

TextCinematicPanel::SpeechState::SpeechState()
{
	this->templateDatKey = -1;
	this->nextVoiceIndex = -1;
}

void TextCinematicPanel::SpeechState::init(int templateDatKey)
{
	this->templateDatKey = templateDatKey;
	this->nextVoiceIndex = 0;
}

bool TextCinematicPanel::SpeechState::isFirstVoice(int voiceIndex)
{
	return voiceIndex == 0;
}

bool TextCinematicPanel::SpeechState::isBeginningOfNewPage(int voiceIndex)
{
	return (voiceIndex % 2) == 0;
}

int TextCinematicPanel::SpeechState::getNextVoiceIndex() const
{
	return this->nextVoiceIndex;
}

std::string TextCinematicPanel::SpeechState::getVoiceFilename(int voiceIndex) const
{
	const int index = voiceIndex / 2;
	const char letter = SpeechState::isBeginningOfNewPage(voiceIndex) ? 'A' : 'B';

	DOSUtils::FilenameBuffer filename;
	std::snprintf(filename.data(), filename.size(), "%d_%02d%c.VOC",
		this->templateDatKey, index, letter);

	return "SPEECH/" + std::string(filename.data());
}

void TextCinematicPanel::SpeechState::incrementVoiceIndex()
{
	this->nextVoiceIndex++;
}

void TextCinematicPanel::SpeechState::resetVoiceIndex()
{
	this->nextVoiceIndex = 0;
}

TextCinematicPanel::TextCinematicPanel(Game &game, int textCinematicDefIndex,
	double secondsPerImage, const std::function<void(Game&)> &endingAction)
	: Panel(game)
{
	const auto &cinematicLibrary = game.getCinematicLibrary();
	const TextCinematicDefinition &textCinematicDef =
		cinematicLibrary.getTextDefinition(textCinematicDefIndex);

	this->textBoxes = [&game, &textCinematicDef]()
	{
		const Int2 center(
			ArenaRenderUtils::SCREEN_WIDTH / 2, 
			ArenaRenderUtils::SCREEN_HEIGHT - 11);

		const auto &textAssetLibrary = game.getTextAssetLibrary();
		const auto &templateDat = textAssetLibrary.getTemplateDat();
		const auto &templateDatEntry = templateDat.getEntry(textCinematicDef.getTemplateDatKey());
		std::string cinematicText = templateDatEntry.values.front();
		cinematicText.push_back('\n');

		// Replace substitution tokens. The original game wraps text onto the next screen if the
		// player's name is too long, which may push the text for every subsequent screen forward
		// by a little bit.
		const std::string playerFirstName = game.getGameData().getPlayer().getFirstName();
		cinematicText = String::replace(cinematicText, "%pcf", playerFirstName);

		// Re-distribute newlines.
		const std::string newText = String::distributeNewlines(cinematicText, 60);

		// Some more formatting should be done in the future so the text wraps nicer. That is,
		// replace all new lines with spaces and redistribute new lines given some max line
		// length value.

		// Count new lines.
		const int newLineCount = static_cast<int>(std::count(newText.begin(), newText.end(), '\n'));

		// Split text into lines.
		const std::vector<std::string> textLines = String::split(newText, '\n');

		// Group up to three text lines per text box.
		std::vector<std::unique_ptr<TextBox>> textBoxes;
		int textBoxesToMake = static_cast<int>(std::ceil(newLineCount / 3)) + 1;
		for (int i = 0; i < textBoxesToMake; i++)
		{
			std::string textBoxText;
			int linesToUse = std::min(newLineCount - (i * 3), 3);
			for (int j = 0; j < linesToUse; j++)
			{
				const std::string &textLine = textLines.at(j + (i * 3));
				textBoxText.append(textLine);
				textBoxText.append("\n");
			}

			// Avoid adding empty text boxes.
			if (textBoxText.size() == 0)
			{
				continue;
			}

			const int lineSpacing = 1;

			// Eventually use a different color for other cinematics (Tharn, Emperor, etc.).
			const auto &fontLibrary = game.getFontLibrary();
			const RichTextString richText(
				textBoxText,
				FontName::Arena,
				textCinematicDef.getFontColor(),
				TextAlignment::Center,
				lineSpacing,
				fontLibrary);

			auto textBox = std::make_unique<TextBox>(
				center, richText, fontLibrary, game.getRenderer());
			textBoxes.push_back(std::move(textBox));
		}

		return textBoxes;
	}();

	this->skipButton = [&endingAction]()
	{
		return Button<Game&>(endingAction);
	}();

	// Optionally initialize speech state if speech is available.
	if (this->shouldPlaySpeech())
	{
		this->speechState.init(textCinematicDef.getTemplateDatKey());
	}

	this->animTextureFilename = textCinematicDef.getAnimationFilename();
	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->textCinematicDefIndex = textCinematicDefIndex;
	this->animImageIndex = 0;
	this->textIndex = 0;
}

bool TextCinematicPanel::shouldPlaySpeech() const
{
	auto &game = this->getGame();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	return !exeData.isFloppyVersion();
}

void TextCinematicPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		// Force the cinematic to end.
		this->skipButton.click(game);

		// Stop voice if it is still playing.
		auto &audioManager = game.getAudioManager();
		audioManager.stopSound();
	}

	// Only allow page skipping if there is no speech.
	if (!this->shouldPlaySpeech())
	{
		bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
		bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);
		bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
			inputManager.keyPressed(e, SDLK_KP_ENTER);

		bool skipHotkeyPressed = spacePressed || enterPressed;

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
		const std::optional<TextureFileMetadata> textureFileMetadata =
			textureManager.tryGetMetadata(this->animTextureFilename.c_str());
		if (!textureFileMetadata.has_value())
		{
			DebugCrash("Couldn't get anim texture file metadata for \"" + this->animTextureFilename + "\".");
		}

		// If at the end of the sequence, go back to the first image. The cinematic ends at the end
		// of the last text box.
		if (this->animImageIndex == textureFileMetadata->getTextureCount())
		{
			this->animImageIndex = 0;
		}
	}

	if (this->shouldPlaySpeech())
	{
		// Update speech state, optionally ending the cinematic if done with last speech.
		auto &game = this->getGame();
		auto &audioManager = game.getAudioManager();

		const bool playedFirstVoice = !SpeechState::isFirstVoice(this->speechState.getNextVoiceIndex());
		if (!playedFirstVoice)
		{
			const std::string voiceFilename =
				this->speechState.getVoiceFilename(this->speechState.getNextVoiceIndex());
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

					if (SpeechState::isBeginningOfNewPage(nextVoiceIndex))
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
