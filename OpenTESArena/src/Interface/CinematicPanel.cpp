#include <cassert>

#include "SDL.h"

#include "CinematicPanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

const double CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE = 1.0 / 20.0;

CinematicPanel::CinematicPanel(GameState *gameState, 
	const std::string &sequenceName, const std::string &paletteName,
	double secondsPerImage, const std::function<void(GameState*)> &endingAction)
	: Panel(gameState)
{
	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->paletteName = paletteName;
	this->sequenceName = sequenceName;
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

	// See if it's time for the next image.
	this->currentSeconds += dt;
	while (this->currentSeconds > this->secondsPerImage)
	{
		this->currentSeconds -= this->secondsPerImage;
		this->imageIndex++;
	}

	auto &textureManager = this->getGameState()->getTextureManager();

	// Get a reference to all images in the sequence.
	const auto &textures = textureManager.getTextures(
		this->sequenceName, this->paletteName);

	// If at the end, then prepare for the next panel.
	if (this->imageIndex >= textures.size())
	{
		this->imageIndex = static_cast<int>(textures.size() - 1);
		this->skipButton->click(this->getGameState());
	}
}

void CinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	auto &textureManager = this->getGameState()->getTextureManager();

	// Get a reference to all images in the sequence.
	const auto &textures = textureManager.getTextures(
		this->sequenceName, this->paletteName);

	// Draw image.
	const auto &texture = textures.at(this->imageIndex);
	renderer.drawToOriginal(texture.get());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
