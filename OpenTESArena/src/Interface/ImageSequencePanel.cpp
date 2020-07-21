#include <algorithm>

#include "SDL.h"

#include "ImageSequencePanel.h"
#include "../Game/Game.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

#include "components/debug/Debug.h"

ImageSequencePanel::ImageSequencePanel(Game &game,
	const std::vector<std::string> &paletteNames,
	const std::vector<std::string> &textureNames,
	const std::vector<double> &imageDurations,
	const std::function<void(Game&)> &endingAction)
	: Panel(game), paletteNames(paletteNames), textureNames(textureNames),
	imageDurations(imageDurations)
{
	DebugAssert(paletteNames.size() == textureNames.size());
	DebugAssert(paletteNames.size() == imageDurations.size());

	this->skipButton = [&endingAction]()
	{
		return Button<Game&>(endingAction);
	}();

	this->currentSeconds = 0.0;
	this->imageIndex = 0;
}

void ImageSequencePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool skipAllHotkeyPressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool skipOneHotkeyPressed = inputManager.keyPressed(e, SDLK_SPACE) ||
		inputManager.keyPressed(e, SDLK_RETURN) || 
		inputManager.keyPressed(e, SDLK_KP_ENTER);

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
	DebugAssertIndex(this->textureNames, this->imageIndex);
	DebugAssertIndex(this->paletteNames, this->imageIndex);
	const std::string &textureName = this->textureNames[this->imageIndex];
	const std::string &paletteName = this->paletteNames[this->imageIndex];
	const TextureID textureID = this->getTextureID(textureName, paletteName);
	const Texture &texture = textureManager.getTexture(textureID);
	renderer.drawOriginal(texture);
}
