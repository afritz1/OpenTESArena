#include <cassert>

#include "SDL.h"

#include "GameWorldPanel.h"

#include "AutomapPanel.h"
#include "Button.h"
#include "CharacterPanel.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "WorldMapPanel.h"
#include "../Entities/CoordinateFrame.h"
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
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"

GameWorldPanel::GameWorldPanel(GameState *gameState)
	: Panel(gameState)
{
	assert(gameState->gameDataIsActive());

	this->automapButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> automapPanel(new AutomapPanel(gameState));
			gameState->setPanel(std::move(automapPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->characterSheetButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> sheetPanel(new CharacterPanel(gameState));
			gameState->setPanel(std::move(sheetPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->logbookButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> logbookPanel(new LogbookPanel(gameState));
			gameState->setPanel(std::move(logbookPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->pauseButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> pausePanel(new PauseMenuPanel(gameState));
			gameState->setPanel(std::move(pausePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->worldMapButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> mapPanel(new WorldMapPanel(gameState));
			gameState->setPanel(std::move(mapPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();
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
			this->pauseButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool activateHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_e);
		bool automapHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_n);
		bool logbookHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_l);
		bool sheetHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_TAB);
		bool worldMapHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_m);

		if (leftClick)
		{
			// Interface buttons? Entities in the world?
		}
		if (activateHotkeyPressed)
		{
			// Activate whatever is looked at.
		}
		else if (automapHotkeyPressed)
		{
			// Go to the automap.
			this->automapButton->click(this->getGameState());
		}
		else if (logbookHotkeyPressed)
		{
			// Go to the logbook.
			this->logbookButton->click(this->getGameState());
		}
		else if (sheetHotkeyPressed)
		{
			// Go to the character screen.
			this->characterSheetButton->click(this->getGameState());
		}
		else if (worldMapHotkeyPressed)
		{
			// Go to the world map.
			this->worldMapButton->click(this->getGameState());
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
		double dxx = static_cast<double>(dx) / static_cast<double>(dimensions.getX());
		double dyy = static_cast<double>(dy) / static_cast<double>(dimensions.getY());

		// Pitch and/or yaw the camera.
		const auto &options = this->getGameState()->getOptions();
		this->getGameState()->getGameData()->getPlayer().rotate(dxx, -dyy,
			options.getHorizontalSensitivity(), options.getVerticalSensitivity(),
			options.getVerticalFOV());
	}
}

void GameWorldPanel::handleKeyboard(double dt)
{
	// Listen for WASD, jump...
	const auto keys = SDL_GetKeyboardState(nullptr);

	bool forward = keys[SDL_SCANCODE_W] != 0;
	bool backward = keys[SDL_SCANCODE_S] != 0;
	bool left = keys[SDL_SCANCODE_A] != 0;
	bool right = keys[SDL_SCANCODE_D] != 0;
	bool jump = keys[SDL_SCANCODE_SPACE] != 0;

	bool any = forward || backward || left || right || jump;

	if (any)
	{
		bool isRunning = keys[SDL_SCANCODE_LSHIFT] != 0;
		auto &player = this->getGameState()->getGameData()->getPlayer();

		// Get some relevant player direction data.
		Float2d groundDirection = player.getGroundDirection();
		Float3d groundDirection3D = Float3d(groundDirection.getX(), 0.0,
			groundDirection.getY()).normalized();
		Float3d rightDirection = player.getFrame().getRight().normalized();

		// Calculate the acceleration direction based on input.
		Float3d accelDirection = Float3d(0.0, 0.0, 0.0);
		if (forward)
		{
			accelDirection = accelDirection + groundDirection3D;
		}
		if (backward)
		{
			accelDirection = accelDirection - groundDirection3D;
		}
		if (right)
		{
			accelDirection = accelDirection + rightDirection;
		}
		if (left)
		{
			accelDirection = accelDirection - rightDirection;
		}

		// To do: check jump eventually once gravity and ground collision are implemented.

		// Use a normalized direction.
		accelDirection = accelDirection.normalized();

		// Set the magnitude of the acceleration to some arbitrary numbers. These values 
		// are independent of max speed. The original game didn't have sprinting, but it 
		// seems like something relevant to do anyway (at least in testing).
		double accelMagnitude = isRunning ? 35.0 : 10.0;

		// Change the player's velocity if valid.
		if (std::isfinite(accelDirection.length()))
		{
			player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
		}
	}
}

void GameWorldPanel::tick(double dt, bool &running)
{
	assert(this->getGameState()->gameDataIsActive());

	this->handleEvents(running);
	this->handleMouse(dt);
	this->handleKeyboard(dt);

	// Animate the game world.
	auto *gameData = this->getGameState()->getGameData();
	gameData->incrementGameTime(dt);

	// Tick the player.
	auto &player = gameData->getPlayer();
	player.tick(this->getGameState(), dt);

	// Update CLProgram members that are refreshed each frame.
	double verticalFOV = this->getGameState()->getOptions().getVerticalFOV();
	auto &clProgram = gameData->getCLProgram();
	clProgram.updateCamera(player.getPosition(), player.getDirection(), verticalFOV);
	clProgram.updateGameTime(gameData->getGameTime());
}

void GameWorldPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Draw game world using OpenCL rendering.
	this->getGameState()->getGameData()->getCLProgram().render(renderer);

	// Interface objects (stat bars, compass, ...) should snap to the edges of the native
	// screen, not just the letterbox, because right now, when the screen is tall, the
	// compass is near the middle of the screen (in the way), and the stat bars are much
	// higher than they should be. I haven't figured out yet what the equation is. I
	// think it requires using the original height and the draw scale somehow.

	// Draw game world interface.
	const auto *gameInterface = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::GameWorldInterface));
	int gameInterfaceWidth, gameInterfaceHeight;
	SDL_QueryTexture(const_cast<SDL_Texture*>(gameInterface), nullptr, nullptr,
		&gameInterfaceWidth, &gameInterfaceHeight);

	this->drawScaledToNative(gameInterface,
		(ORIGINAL_WIDTH / 2) - (gameInterfaceWidth / 2),
		ORIGINAL_HEIGHT - gameInterfaceHeight,
		gameInterfaceWidth,
		gameInterfaceHeight,
		renderer);

	// Compass frame.
	const auto &compassFrame = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CompassFrame));
	SDL_SetColorKey(compassFrame.getSurface(), SDL_TRUE, Color::Black.toRGB());

	// Compass slider (the actual headings). +X is north, +Z is east.
	const auto &compassSlider = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CompassSlider));

	Surface compassSliderSegment(32, 7);
	compassSlider.blit(compassSliderSegment, Int2(), Rect(60, 0,
		compassSliderSegment.getWidth(), compassSliderSegment.getHeight()));

	// Should do some sin() and cos() functions to get the segment location.
	//int segmentX = ...;

	// Fill in transparent edges behind compass slider (due to SDL blit truncation).
	Surface compassFiller(36, 11);
	compassFiller.fill(Color(205, 186, 155));

	this->drawScaledToNative(compassFiller,
		(ORIGINAL_WIDTH / 2) - (compassFrame.getWidth() / 2) + 1,
		5,
		compassFiller.getWidth(),
		compassFiller.getHeight(),
		renderer);

	this->drawScaledToNative(compassSliderSegment,
		(ORIGINAL_WIDTH / 2) - 16,
		7,
		compassSliderSegment.getWidth(),
		compassSliderSegment.getHeight(),
		renderer);

	this->drawScaledToNative(compassFrame,
		(ORIGINAL_WIDTH / 2) - (compassFrame.getWidth() / 2),
		0,
		compassFrame.getWidth(),
		compassFrame.getHeight(),
		renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
