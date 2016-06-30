#include <cassert>

#include "SDL.h"

#include "ImagePanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

ImagePanel::ImagePanel(GameState *gameState, TextureName textureName,
	double secondsToDisplay, const std::function<void(GameState*)> &endingAction)
	: Panel(gameState)
{
	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->textureName = textureName;
	this->secondsToDisplay = secondsToDisplay;
	this->currentSeconds = 0.0;
}

ImagePanel::~ImagePanel()
{

}

void ImagePanel::handleEvents(bool &running)
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
		bool skipHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_SPACE) ||
				(e.key.keysym.sym == SDLK_RETURN) ||
				(e.key.keysym.sym == SDLK_ESCAPE) ||
				(e.key.keysym.sym == SDLK_KP_ENTER));

		if (leftClick || skipHotkeyPressed)
		{
			this->skipButton->click(this->getGameState());
		}
	}
}

void ImagePanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ImagePanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ImagePanel::tick(double dt, bool &running)
{
	this->handleEvents(running);
	this->currentSeconds += dt;
	if (this->currentSeconds > this->secondsToDisplay)
	{
		this->skipButton->click(this->getGameState());
	}
}

void ImagePanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Draw image.
	const auto *image = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(this->textureName));
	this->drawLetterbox(image, renderer, letterbox);
}
