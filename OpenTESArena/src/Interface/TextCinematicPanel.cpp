#include <algorithm>
#include <cmath>
#include <cstdio>

#include "TextCinematicPanel.h"
#include "TextCinematicUiView.h"
#include "../Audio/AudioManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Math/Vector2.h"
#include "../Media/TextCinematicDefinition.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

#include "components/debug/Debug.h"

TextCinematicPanel::TextCinematicPanel(Game &game)
	: Panel(game) { }

TextCinematicPanel::~TextCinematicPanel()
{
	// Stop voice if it is still playing.
	auto &game = this->getGame();
	auto &audioManager = game.getAudioManager();
	audioManager.stopSound();
}

bool TextCinematicPanel::init(int textCinematicDefIndex, double secondsPerImage, const OnFinishedFunction &onFinished)
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &cinematicLibrary = game.getCinematicLibrary();
	const auto &fontLibrary = game.getFontLibrary();

	const TextCinematicDefinition &textCinematicDef = cinematicLibrary.getTextDefinition(textCinematicDefIndex);
	const TextBox::InitInfo subtitlesTextBoxInitInfo = TextCinematicUiView::getSubtitlesTextBoxInitInfo(textCinematicDef.getFontColor(), fontLibrary);
	if (!this->textBox.init(subtitlesTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init subtitles text box.");
		return false;
	}

	auto skipPageFunc = [this, onFinished](Game &game)
	{
		// Only allow page skipping if there is no speech.
		if (!TextCinematicUiModel::shouldPlaySpeech(game))
		{
			this->textIndex++;

			// If done with the last text box, then prepare for the next panel.
			int textBoxCount = static_cast<int>(this->textPages.size());
			if (this->textIndex >= textBoxCount)
			{
				this->textIndex = textBoxCount - 1;
				onFinished(game);
			}

			this->updateSubtitles();
		}
	};

	this->skipButton = Button<Game&>(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT,
		[skipPageFunc](Game &game) { skipPageFunc(game); });

	this->addButtonProxy(MouseButtonType::Left, this->skipButton.getRect(),
		[this, &game]() { this->skipButton.click(game); });

	this->addInputActionListener(InputActionName::Skip,
		[onFinished, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			onFinished(game);
		}
	});

	const std::string subtitleText = TextCinematicUiModel::getSubtitleText(game, textCinematicDef);
	this->textPages = TextCinematicUiModel::getSubtitleTextPages(subtitleText);

	auto &textureManager = game.getTextureManager();
	const std::string &animFilename = textCinematicDef.getAnimationFilename();
	const std::vector<UiTextureID> animTextureIDs = TextCinematicUiView::allocAnimationTextures(animFilename, textureManager, renderer);
	if (animTextureIDs.size() == 0)
	{
		DebugLogError("No animation frames for text cinematic \"" + animFilename + "\".");
		return false;
	}

	for (const UiTextureID animTextureID : animTextureIDs)
	{
		ScopedUiTextureRef animTextureRef;
		animTextureRef.init(animTextureID, renderer);
		this->animTextureRefs.emplace_back(std::move(animTextureRef));
	}

	UiDrawCall::TextureFunc animTextureFunc = [this]()
	{
		DebugAssertIndex(this->animTextureRefs, this->animImageIndex);
		return this->animTextureRefs[this->animImageIndex].get();
	};

	const std::optional<Int2> animTextureDims = renderer.tryGetUiTextureDims(animTextureIDs.front());
	DebugAssert(animTextureDims.has_value());
	this->addDrawCall(
		animTextureFunc,
		Int2::Zero,
		*animTextureDims,
		PivotType::TopLeft);

	const Rect &textBoxRect = this->textBox.getRect();
	this->addDrawCall(
		[this]() { return this->textBox.getTextureID(); },
		textBoxRect.getCenter(),
		Int2(textBoxRect.getWidth(), textBoxRect.getHeight()),
		PivotType::Middle);

	// Optionally initialize speech state if speech is available.
	if (TextCinematicUiModel::shouldPlaySpeech(game))
	{
		this->speechState.init(textCinematicDef.getTemplateDatKey());
	}

	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->textCinematicDefIndex = textCinematicDefIndex;
	this->animImageIndex = 0;
	this->textIndex = 0;
	this->updateSubtitles();

	return true;
}

void TextCinematicPanel::updateSubtitles()
{
	std::string textToDisplay;
	if (this->textIndex < static_cast<int>(this->textPages.size()))
	{
		textToDisplay = this->textPages[this->textIndex];
	}

	this->textBox.setText(textToDisplay);
}

void TextCinematicPanel::tick(double dt)
{
	this->currentImageSeconds += dt;

	while (this->currentImageSeconds > this->secondsPerImage)
	{
		this->currentImageSeconds -= this->secondsPerImage;
		this->animImageIndex++;

		// If at the end of the sequence, go back to the first image. The cinematic ends at the end
		// of the last text box.
		if (this->animImageIndex == static_cast<int>(this->animTextureRefs.size()))
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
						this->updateSubtitles();
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
