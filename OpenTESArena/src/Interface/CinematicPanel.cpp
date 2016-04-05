#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "CinematicPanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/TextureManager.h"

const double CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE = 1.0 / 20.0;

CinematicPanel::CinematicPanel(GameState *gameState, TextureSequenceName name, 
	double secondsPerImage, const std::function<void()> &endingAction)
	: Panel(gameState)
{
	this->skipButton = nullptr;

	this->skipButton = [gameState, &endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->sequenceName = name;
	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;

	assert(this->skipButton.get() != nullptr);
	assert(this->sequenceName == name);
	assert(this->secondsPerImage == secondsPerImage);
	assert(this->currentSeconds == 0.0);
	assert(this->imageIndex == 0);
}

CinematicPanel::~CinematicPanel()
{

}

void CinematicPanel::handleEvents(bool &running)
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
			((e.key.keysym.sym == SDLK_SPACE) || (e.key.keysym.sym == SDLK_RETURN) ||
				(e.key.keysym.sym == SDLK_ESCAPE));

		if (leftClick || skipHotkeyPressed)
		{
			this->skipButton->click();
		}
	}
}

void CinematicPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void CinematicPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void CinematicPanel::tick(double dt, bool &running)
{
	this->handleEvents(running);

	this->currentSeconds += dt;
	while (this->currentSeconds > this->secondsPerImage)
	{
		this->currentSeconds -= this->secondsPerImage;
		this->imageIndex++;
	}
}

void CinematicPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	const auto &images = this->getGameState()->getTextureManager()
		.getSequence(this->sequenceName);

	// If at the end, then prepare for the next panel.
	if (this->imageIndex >= images.size())
	{
		this->imageIndex = static_cast<int>(images.size() - 1);
		this->skipButton->click();
	}

	// Draw image.
	const auto &image = images.at(this->imageIndex);
	this->drawLetterbox(image, dst, letterbox);
}
