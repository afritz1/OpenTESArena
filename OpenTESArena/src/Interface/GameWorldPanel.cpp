#include <cassert>
#include <iostream>

#include "SDL.h"

#include "GameWorldPanel.h"

#include "Button.h"
#include "CharacterPanel.h"
#include "PauseMenuPanel.h"

#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Math/Random.h"
#include "../Math/Rect.h"
#include "../Media/AudioManager.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/SoundName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/CLProgram.h"

GameWorldPanel::GameWorldPanel(GameState *gameState)
	: Panel(gameState)
{
	assert(gameState->gameDataIsActive());

	this->characterSheetButton = nullptr;
	this->pauseButton = nullptr;

	this->characterSheetButton = [gameState]()
	{
		auto function = [gameState]()
		{
			auto sheetPanel = std::unique_ptr<Panel>(new CharacterPanel(gameState));
			gameState->setPanel(std::move(sheetPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->pauseButton = [gameState]()
	{
		auto function = [gameState]()
		{
			auto pausePanel = std::unique_ptr<Panel>(new PauseMenuPanel(gameState));
			gameState->setPanel(std::move(pausePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	assert(this->characterSheetButton.get() != nullptr);
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

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool activateHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_e);
		bool sheetHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_TAB);

		if (leftClick)
		{
			// Attack...
			// Just play a sound for now.
		}
		if (activateHotkeyPressed)
		{
			// Activate whatever is looked at.
		}
		if (sheetHotkeyPressed)
		{
			// Go to character screen.
			this->characterSheetButton->click();
		}
	}
}

void GameWorldPanel::handleMouse(double dt)
{
	static_cast<void>(dt);

	// Make the camera look around.
	int dx, dy;
	const auto mouse = SDL_GetRelativeMouseState(&dx, &dy);

	// The program will eventually not require holding a mouse button to turn.
	bool leftClick = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	bool rightClick = (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	bool turning = ((dx != 0) || (dy != 0)) && (leftClick || rightClick);

	if (turning)
	{
		auto dimensions = this->getGameState()->getScreenDimensions();
		auto dxx = static_cast<double>(dx) / static_cast<double>(dimensions.getX());
		auto dyy = static_cast<double>(dy) / static_cast<double>(dimensions.getY());

		// Pitch and/or yaw the camera.
		const auto &options = this->getGameState()->getOptions();
		this->getGameState()->getGameData()->getPlayer().rotate(dxx, -dyy,
			options.getHorizontalSensitivity(), options.getVerticalSensitivity(),
			options.getVerticalFOV());
	}
}

void GameWorldPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
	// Listen for WASD, jump, crouch...
}

void GameWorldPanel::tick(double dt, bool &running)
{
	this->handleEvents(running);
	this->handleMouse(dt);

	// Animate the game world by "dt" seconds here.

	// Get zoom of the player's camera. A zoom of 1.0 means FOV of 90.0.
	auto zoom = 1.0 / std::tan(this->getGameState()->getOptions()
		.getVerticalFOV() * 0.5 * DEG_TO_RAD);

	// Update CLProgram members that are refreshed per frame.
	auto *gameData = this->getGameState()->getGameData();
	auto direction = gameData->getPlayer().getDirection();
	gameData->getCLProgram().updateDirection(direction.scaledBy(zoom));
}

void GameWorldPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Draw game world (OpenCL rendering). No need to clear the screen beforehand.
	this->getGameState()->getGameData()->getCLProgram().render(dst);

	// Interface objects (stat bars, compass, ...) should snap to the edges of the native
	// screen, not just the letterbox, because right now, when the screen is tall, the
	// compass is near the middle of the screen (in the way), and the stat bars are much
	// higher than they should be. I haven't figured out yet what the equation is. I
	// think it requires using the original height and the draw scale somehow.

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

	// Compass frame.
	const auto &compassFrame = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CompassFrame));
	SDL_SetColorKey(compassFrame.getSurface(), SDL_TRUE, this->getMagenta(dst->format));

	// Compass slider (the actual headings). +X is north, +Z is east.
	const auto &compassSlider = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CompassSlider));

	auto compassSliderSegment = Surface(32, 7);
	compassSlider.blit(compassSliderSegment, Int2(), Rect(60, 0,
		compassSliderSegment.getWidth(), compassSliderSegment.getHeight()));

	// Should do some sin() and cos() functions to get the segment location.
	//int segmentX = ...;

	// Fill in transparent edges behind compass slider (due to SDL blit truncation).
	auto compassFiller = Surface(36, 11);
	compassFiller.fill(Color(205, 186, 155));

	this->drawScaledToNative(compassFiller,
		(ORIGINAL_WIDTH / 2) - (compassFrame.getWidth() / 2) + 1,
		5,
		compassFiller.getWidth(),
		compassFiller.getHeight(),
		dst);

	this->drawScaledToNative(compassSliderSegment,
		(ORIGINAL_WIDTH / 2) - 16,
		7,
		compassSliderSegment.getWidth(),
		compassSliderSegment.getHeight(),
		dst);

	this->drawScaledToNative(compassFrame,
		(ORIGINAL_WIDTH / 2) - (compassFrame.getWidth() / 2),
		0,
		compassFrame.getWidth(),
		compassFrame.getHeight(),
		dst);

	// Draw cursor for now. It won't be drawn once the game world is developed enough.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
