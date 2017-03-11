#include <algorithm>
#include <cassert>

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

ImageSequencePanel::ImageSequencePanel(Game *game,
	const std::vector<std::string> &paletteNames,
	const std::vector<std::string> &textureNames,
	const std::vector<double> &imageDurations,
	const std::function<void(Game*)> &endingAction)
	: Panel(game), paletteNames(paletteNames), textureNames(textureNames),
	imageDurations(imageDurations)
{
	assert(paletteNames.size() == textureNames.size());
	assert(paletteNames.size() == imageDurations.size());

	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button<Game*>>(new Button<Game*>(endingAction));
	}();

	this->currentSeconds = 0.0;
	this->imageIndex = 0;
}

ImageSequencePanel::~ImageSequencePanel()
{

}

void ImageSequencePanel::handleEvent(const SDL_Event &e)
{
	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);
	bool skipAllHotkeyPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool skipOneHotkeyPressed = (e.type == SDL_KEYDOWN) &&
		((e.key.keysym.sym == SDLK_SPACE) ||
		(e.key.keysym.sym == SDLK_RETURN) ||
			(e.key.keysym.sym == SDLK_KP_ENTER));

	if (skipAllHotkeyPressed)
	{
		this->skipButton->click(this->getGame());
	}
	else if (leftClick || skipOneHotkeyPressed)
	{
		this->currentSeconds = 0.0;

		const int imageCount = static_cast<int>(this->textureNames.size());

		this->imageIndex = std::min(this->imageIndex + 1, imageCount);

		if (this->imageIndex == imageCount)
		{
			this->skipButton->click(this->getGame());
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
				this->skipButton->click(this->getGame());
			}
		}
	}

	// Clamp against the max so the index doesn't go outside the image vector.
	this->imageIndex = std::min(this->imageIndex, imageCount - 1);
}

void ImageSequencePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	auto &textureManager = this->getGame()->getTextureManager();

	// Draw image.
	const auto &image = textureManager.getTexture(
		this->textureNames.at(this->imageIndex),
		this->paletteNames.at(this->imageIndex));
	renderer.drawToOriginal(image.get());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
