#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "ImageSequencePanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

ImageSequencePanel::ImageSequencePanel(GameState *gameState,
	const std::vector<std::string> &paletteNames,
	const std::vector<std::string> &textureNames,
	const std::vector<double> &imageDurations,
	const std::function<void(GameState*)> &endingAction)
	: Panel(gameState), paletteNames(paletteNames), textureNames(textureNames),
	imageDurations(imageDurations)
{
	assert(paletteNames.size() == textureNames.size());
	assert(paletteNames.size() == imageDurations.size());

	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->currentSeconds = 0.0;
	this->imageIndex = 0;
}

ImageSequencePanel::~ImageSequencePanel()
{

}

void ImageSequencePanel::handleEvents(bool &running)
{
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}

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
			this->skipButton->click(this->getGameState());
		}
		else if (leftClick || skipOneHotkeyPressed)
		{
			this->currentSeconds = 0.0;

			const int imageCount = static_cast<int>(this->textureNames.size());

			this->imageIndex = std::min(this->imageIndex + 1, imageCount);

			if (this->imageIndex == imageCount)
			{
				this->skipButton->click(this->getGameState());
			}
		}
	}
}

void ImageSequencePanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ImageSequencePanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ImageSequencePanel::tick(double dt, bool &running)
{
	this->handleEvents(running);

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
				this->skipButton->click(this->getGameState());
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

	auto &textureManager = this->getGameState()->getTextureManager();

	// Draw image.
	auto *image = textureManager.getTexture(
		this->textureNames.at(this->imageIndex),
		this->paletteNames.at(this->imageIndex));
	renderer.drawToOriginal(image);

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
