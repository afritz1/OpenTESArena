#include <cassert>
#include <iostream>
#include <cmath>

#include <SDL2/SDL.h>

#include "Game.h"

#include "GameState.h"

const int Game::DEFAULT_FPS = 60;

Game::Game()
{
	this->gameState = std::unique_ptr<GameState>(new GameState());
	this->targetFPS = Game::DEFAULT_FPS;
}

Game::~Game()
{

}

void Game::delay(int ms)
{
	assert(ms >= 0);
	SDL_Delay(static_cast<Uint32>(ms));
}

void Game::loop()
{
	// This loop doesn't check for SDL events itself. The current panel does that,
	// because most events like pressing "Esc" are context-sensitive.

	int startTime = SDL_GetTicks();
	int endTime = SDL_GetTicks();

	while (this->gameState->isRunning())
	{
		startTime = SDL_GetTicks();

		int minimumMS = 1000 / this->targetFPS;
		double dt = std::fmax(startTime - endTime, minimumMS) / 1000.0;

		this->gameState->tick(dt);
		this->gameState->render();

		endTime = SDL_GetTicks();

		int frameTime = endTime - startTime;
		if (frameTime < minimumMS)
		{
			this->delay(minimumMS - frameTime);
		}
	}
}
