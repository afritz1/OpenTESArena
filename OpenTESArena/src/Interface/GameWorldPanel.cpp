#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "GameWorldPanel.h"

#include "Button.h"
#include "PauseMenuPanel.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

GameWorldPanel::GameWorldPanel(GameState *gameState)
	: Panel(gameState)
{
	this->pauseButton = nullptr;

	this->pauseButton = [gameState]()
	{
		auto function = [gameState]()
		{
			auto pausePanel = std::unique_ptr<Panel>(new PauseMenuPanel(gameState));
			gameState->setPanel(std::move(pausePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	assert(this->pauseButton.get() != nullptr);
}

GameWorldPanel::~GameWorldPanel()
{

}

void GameWorldPanel::handleEvents(bool &running)
{
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);

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
		if (escapePressed)
		{
			this->pauseButton->click();
		}

		// Listen for left click to attack...		
		// Listen for "activate" key, probably E.
		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool activateHotkeyPressed = (e.type == SDL_KEYDOWN) && 
			(e.key.keysym.sym == SDLK_e);

		if (leftClick)
		{

		}
		if (activateHotkeyPressed) 
		{

		}
	}
}

void GameWorldPanel::handleMouse(double dt)
{
	// Make camera look around.
	// The code for this is already in another project. I just need to bring it over
	// and make a couple changes for having the window grab the mouse.
}

void GameWorldPanel::handleKeyboard(double dt)
{
	// Listen for WASD, jump, crouch...
}

void GameWorldPanel::tick(double dt, bool &running)
{
	// Animate the game world by "dt" seconds...
	
	this->handleEvents(running);
}

void GameWorldPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	const int originalWidth = 320;
	const int originalHeight = 200;

	// Temporary background.
	SDL_FillRect(dst, letterbox, SDL_MapRGB(dst->format, 0, 0, 0));

	// Draw game world (OpenCL rendering)...

	// Draw stat bars.
	auto statBarSurface = Surface(5, 35);
	
	statBarSurface.fill(Color(0, 255, 0));
	this->drawScaledToNative(statBarSurface, 
		5, 
		160, 
		statBarSurface.getWidth(), 
		statBarSurface.getHeight(),
		dst);

	statBarSurface.fill(Color(255, 0, 0));
	this->drawScaledToNative(statBarSurface, 
		13, 
		160, 
		statBarSurface.getWidth(),
		statBarSurface.getHeight(), 
		dst);

	statBarSurface.fill(Color(0, 0, 255));
	this->drawScaledToNative(statBarSurface,
		21,
		160,
		statBarSurface.getWidth(),
		statBarSurface.getHeight(),
		dst);
	
	// Draw compass.
	const auto &compassFrame = this->getGameState()->getTextureManager()
		.getSurface(TextureName::CompassFrame);
	SDL_SetColorKey(compassFrame.getSurface(), SDL_TRUE, this->getMagenta(dst->format));

	this->drawScaledToNative(compassFrame,
		(originalWidth / 2) - (compassFrame.getWidth() / 2),
		0,
		compassFrame.getWidth(),
		compassFrame.getHeight(),
		dst);

	// Draw cursor for now. It won't be drawn once the game world is developed enough.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureName::SwordCursor);
	this->drawCursor(cursor, dst);
}
