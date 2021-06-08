#include <algorithm>

#include "SDL.h"

#include "ImageSequencePanel.h"
#include "../Game/Game.h"
#include "../Media/TextureManager.h"
#include "../UI/Texture.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

ImageSequencePanel::ImageSequencePanel(Game &game)
	: Panel(game) { }

bool ImageSequencePanel::init(const std::vector<std::string> &paletteNames,
	const std::vector<std::string> &textureNames, const std::vector<double> &imageDurations,
	const std::function<void(Game&)> &endingAction)
{
	if (paletteNames.size() != textureNames.size())
	{
		DebugLogError("Palette names size (" + std::to_string(paletteNames.size()) +
			") doesn't match texture names size (" + std::to_string(textureNames.size()) + ").");
		return false;
	}

	if (paletteNames.size() != imageDurations.size())
	{
		DebugLogError("Palette names size (" + std::to_string(paletteNames.size()) +
			") doesn't match image durations size (" + std::to_string(imageDurations.size()) + ").");
		return false;
	}

	this->skipButton = Button<Game&>(endingAction);
	this->paletteNames = paletteNames;
	this->textureNames = textureNames;
	this->imageDurations = imageDurations;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
	return true;
}

void ImageSequencePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool skipAllHotkeyPressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool skipOneHotkeyPressed = inputManager.keyPressed(e, SDLK_SPACE) ||
		inputManager.keyPressed(e, SDLK_RETURN) || inputManager.keyPressed(e, SDLK_KP_ENTER);

	if (skipAllHotkeyPressed)
	{
		this->skipButton.click(this->getGame());
	}
	else if (leftClick || skipOneHotkeyPressed)
	{
		this->currentSeconds = 0.0;

		const int imageCount = static_cast<int>(this->textureNames.size());
		this->imageIndex = std::min(this->imageIndex + 1, imageCount);

		if (this->imageIndex == imageCount)
		{
			this->skipButton.click(this->getGame());
		}
	}	
}

void ImageSequencePanel::tick(double dt)
{
	const int imageCount = static_cast<int>(this->textureNames.size());

	// Check if done iterating through images.
	if (this->imageIndex < imageCount)
	{
		this->currentSeconds += dt;

		// Step to the next image if its duration has passed.
		if (this->currentSeconds >= this->imageDurations.at(this->imageIndex))
		{
			this->currentSeconds = 0.0;
			this->imageIndex++;

			// Check if the last image is now over.
			if (this->imageIndex == imageCount)
			{
				this->skipButton.click(this->getGame());
			}
		}
	}

	// Clamp against the max so the index doesn't go outside the image vector.
	this->imageIndex = std::min(this->imageIndex, imageCount - 1);
}

void ImageSequencePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw image.
	auto &textureManager = this->getGame().getTextureManager();
	DebugAssertIndex(this->paletteNames, this->imageIndex);
	const std::string &paletteName = this->paletteNames[this->imageIndex];
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteName + "\".");
		return;
	}

	DebugAssertIndex(this->textureNames, this->imageIndex);
	const std::string &textureName = this->textureNames[this->imageIndex];
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureName.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + textureName + "\".");
		return;
	}

	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);
}
