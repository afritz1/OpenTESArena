#include <cassert>

#include "SDL.h"

#include "CinematicPanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureSequenceName.h"

const double CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE = 1.0 / 20.0;

CinematicPanel::CinematicPanel(GameState *gameState, PaletteName paletteName, 
	TextureSequenceName name, double secondsPerImage, 
	const std::function<void(GameState*)> &endingAction)
	: Panel(gameState)
{
	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->paletteName = paletteName;
	this->sequenceName = name;
	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
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

void CinematicPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Get all of the image filenames relevant to the sequence.
	std::vector<std::string> filenames = TextureFile::fromName(this->sequenceName);

	// If at the end, then prepare for the next panel.
	if (this->imageIndex >= filenames.size())
	{
		this->imageIndex = static_cast<int>(filenames.size() - 1);
		this->skipButton->click(this->getGameState());
	}

	auto &textureManager = this->getGameState()->getTextureManager();

	// Draw image.
	const auto *image = textureManager.getTexture(
		filenames.at(this->imageIndex), this->paletteName);
	this->drawLetterbox(image, renderer, letterbox);
}
