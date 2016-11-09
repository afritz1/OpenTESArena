#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "GameWorldPanel.h"

#include "AutomapPanel.h"
#include "Button.h"
#include "CharacterPanel.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Math/CoordinateFrame.h"
#include "../Math/Int2.h"
#include "../Math/Random.h"
#include "../Math/Rect.h"
#include "../Media/AudioManager.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/SoundName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/CLProgram.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"

namespace
{
	// Original arrow cursor rectangles for each part of the letterbox. Their
	// components can be multiplied by the ratio of the native and the original
	// resolution so they're flexible with most resolutions.
	const Rect TopLeftRegion(0, 0, 141, 49);
	const Rect TopMiddleRegion(141, 0, 38, 49);
	const Rect TopRightRegion(179, 0, 141, 49);
	const Rect MiddleLeftRegion(0, 49, 90, 70);
	const Rect MiddleRegion(90, 49, 140, 70);
	const Rect MiddleRightRegion(230, 49, 90, 70);
	const Rect BottomLeftRegion(0, 119, 141, 28);
	const Rect BottomMiddleRegion(141, 119, 38, 28);
	const Rect BottomRightRegion(179, 119, 141, 28);
	const Rect UiBottomRegion(0, 147, 320, 53);

	// UI button regions.
	const Rect PortraitRegion(14, 166, 40, 29);
	const Rect DrawWeaponRegion(88, 151, 29, 22);
	const Rect MapRegion(118, 151, 29, 22);
	const Rect ThievingRegion(147, 151, 29, 22);
	const Rect StatusRegion(177, 151, 29, 22);
	const Rect MagicRegion(88, 175, 29, 22);
	const Rect LogbookRegion(118, 175, 29, 22);
	const Rect UseItemRegion(147, 175, 29, 22);
	const Rect RestRegion(177, 175, 29, 22);

	// Magic and use item scroll buttons, relative to the top left of the interface 
	// (not programmed until later).
	const Rect ScrollUpRegion(208, 3, 9, 9);
	const Rect ScrollDownRegion(208, 42, 9, 9);
}

GameWorldPanel::GameWorldPanel(GameState *gameState)
	: Panel(gameState)
{
	assert(gameState->gameDataIsActive());

	this->playerNameTextBox = [gameState]()
	{
		int x = 17;
		int y = 154;
		Color color(215, 121, 8);
		std::string text = gameState->getGameData()->getPlayer().getFirstName();
		auto &font = gameState->getFontManager().getFont(FontName::Char);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

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

	// Set all of the cursor regions relative to the current window.
	const Int2 screenDims = gameState->getRenderer().getWindowDimensions();
	this->updateCursorRegions(screenDims.getX(), screenDims.getY());
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
			this->updateCursorRegions(width, height);
		}
		if (escapePressed)
		{
			this->pauseButton->click(this->getGameState());
		}

		bool takeScreenshot = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_PRINTSCREEN);

		if (takeScreenshot)
		{
			auto &renderer = this->getGameState()->getRenderer();
			SDL_Surface *screenshot = renderer.getScreenshot();
			SDL_SaveBMP(screenshot, "out.bmp");
			SDL_FreeSurface(screenshot);
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_RIGHT);

		const auto &renderer = this->getGameState()->getRenderer();

		// Get mouse position relative to letterbox coordinates.
		const Int2 originalPosition = renderer.nativePointToOriginal(
			this->getMousePosition());

		if (leftClick)
		{
			// Was an interface button clicked?
			if (PortraitRegion.contains(originalPosition))
			{
				this->characterSheetButton->click(this->getGameState());
			}
			else if (DrawWeaponRegion.contains(originalPosition))
			{
				Debug::mention("Game", "Draw weapon.");
			}
			else if (MapRegion.contains(originalPosition))
			{
				this->automapButton->click(this->getGameState());
			}
			else if (ThievingRegion.contains(originalPosition))
			{
				Debug::mention("Game", "Thieving.");
			}
			else if (StatusRegion.contains(originalPosition))
			{
				Debug::mention("Game", "Status.");
			}
			else if (MagicRegion.contains(originalPosition))
			{
				Debug::mention("Game", "Magic.");
			}
			else if (LogbookRegion.contains(originalPosition))
			{
				this->logbookButton->click(this->getGameState());
			}
			else if (UseItemRegion.contains(originalPosition))
			{
				Debug::mention("Game", "Use item.");
			}
			else if (RestRegion.contains(originalPosition))
			{
				Debug::mention("Game", "Rest.");
			}

			// Later... any entities in the world clicked?
		}
		else if (rightClick)
		{
			if (MapRegion.contains(originalPosition))
			{
				this->worldMapButton->click(this->getGameState());
			}
		}

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

		if (activateHotkeyPressed)
		{
			// Activate whatever is looked at.
		}
		else if (automapHotkeyPressed)
		{
			this->automapButton->click(this->getGameState());
		}
		else if (logbookHotkeyPressed)
		{
			this->logbookButton->click(this->getGameState());
		}
		else if (sheetHotkeyPressed)
		{
			this->characterSheetButton->click(this->getGameState());
		}
		else if (worldMapHotkeyPressed)
		{
			this->worldMapButton->click(this->getGameState());
		}
	}
}

void GameWorldPanel::handleMouse(double dt)
{
	static_cast<void>(dt);

	const auto &renderer = this->getGameState()->getRenderer();

	const uint32_t mouse = SDL_GetRelativeMouseState(nullptr, nullptr);
	const bool leftClick = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

	if (leftClick)
	{
		// Horizontal camera movement rough draft. The original camera controls for 
		// Arena are bad, but I am simulating them before thinking of adding modern 
		// 3D camera support (like Daggerfall) as an option.
		const Int2 screenDimensions = renderer.getWindowDimensions();
		const Int2 mousePosition = this->getMousePosition();

		// Strength of turning is determined by proximity of the mouse cursor to
		// the left or right screen edge.
		const double dx = [this, &mousePosition, &screenDimensions]()
		{
			const int mouseX = mousePosition.getX();

			// Native cursor regions (scaled to the current window).
			const Rect &middleLeft = *this->nativeCursorRegions.at(3).get();
			const Rect &middleRight = *this->nativeCursorRegions.at(5).get();

			// Measure the magnitude of rotation. -1.0 is left, 1.0 is right.
			double percent = 0.0;
			if (middleLeft.contains(mousePosition))
			{
				percent = -1.0 + (static_cast<double>(mouseX) / middleLeft.getWidth());
			}
			else if (middleRight.contains(mousePosition))
			{
				percent = static_cast<double>(mouseX - middleRight.getLeft()) /
					middleRight.getWidth();
			}

			// Reduce the magnitude by a lot as a baseline. Sensitivity can be 
			// tweaked in the options.
			percent *= 0.010;

			// No NaNs or infinities allowed.
			return std::isfinite(percent) ? percent : 0.0;
		}();

		auto &player = this->getGameState()->getGameData()->getPlayer();
		const auto &options = this->getGameState()->getOptions();

		// Yaw the camera left or right. No vertical movement in classic camera mode.
		player.rotate(dx, 0.0, options.getHorizontalSensitivity(),
			options.getVerticalSensitivity(), options.getVerticalFOV());
	}

	// Later in development, a 3D camera would be fun (more like Daggerfall), but 
	// for now the objective is to more closely resemble the original game, so the
	// rough draft 3D camera code below is commented out as a result.

	// Make the camera look around.
	/*int dx, dy;
	const auto mouse = SDL_GetRelativeMouseState(&dx, &dy);

	bool leftClick = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	bool rightClick = (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	bool turning = ((dx != 0) || (dy != 0)) && leftClick;

	if (turning)
	{
		auto dimensions = this->getGameState()->getRenderer().getWindowDimensions();
		double dxx = static_cast<double>(dx) / static_cast<double>(dimensions.getX());
		double dyy = static_cast<double>(dy) / static_cast<double>(dimensions.getY());

		// Pitch and/or yaw the camera.
		const auto &options = this->getGameState()->getOptions();
		this->getGameState()->getGameData()->getPlayer().rotate(dxx, -dyy,
			options.getHorizontalSensitivity(), options.getVerticalSensitivity(),
			options.getVerticalFOV());
	}*/
}

void GameWorldPanel::handleKeyboard(double dt)
{
	// Listen for WASD, jump...
	const auto *keys = SDL_GetKeyboardState(nullptr);

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
		double accelMagnitude = isRunning ? 30.0 : 10.0;

		// Change the player's velocity if valid.
		if (std::isfinite(accelDirection.length()))
		{
			player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
		}
	}
}

Float2d GameWorldPanel::getMotionMagnitudes(const Int2 &nativePoint)
{
	// To do...
	return Float2d();
}

void GameWorldPanel::updateCursorRegions(int width, int height)
{
	// Scale ratios.
	const double xScale = static_cast<double>(width) /
		static_cast<double>(Renderer::ORIGINAL_WIDTH);
	const double yScale = static_cast<double>(height) /
		static_cast<double>(Renderer::ORIGINAL_HEIGHT);

	// Lambda for making a cursor region that scales to the current resolution.
	auto scaleRect = [xScale, yScale](const Rect &rect)
	{
		const int x = static_cast<int>(std::ceil(
			static_cast<double>(rect.getLeft()) * xScale));
		const int y = static_cast<int>(std::ceil(
			static_cast<double>(rect.getTop()) * yScale));
		const int width = static_cast<int>(std::ceil(
			static_cast<double>(rect.getWidth()) * xScale));
		const int height = static_cast<int>(std::ceil(
			static_cast<double>(rect.getHeight()) * yScale));

		return std::unique_ptr<Rect>(new Rect(x, y, width, height));
	};

	// Top row.
	this->nativeCursorRegions.at(0) = scaleRect(TopLeftRegion);
	this->nativeCursorRegions.at(1) = scaleRect(TopMiddleRegion);
	this->nativeCursorRegions.at(2) = scaleRect(TopRightRegion);

	// Middle row.
	this->nativeCursorRegions.at(3) = scaleRect(MiddleLeftRegion);
	this->nativeCursorRegions.at(4) = scaleRect(MiddleRegion);
	this->nativeCursorRegions.at(5) = scaleRect(MiddleRightRegion);

	// Bottom row.
	this->nativeCursorRegions.at(6) = scaleRect(BottomLeftRegion);
	this->nativeCursorRegions.at(7) = scaleRect(BottomMiddleRegion);
	this->nativeCursorRegions.at(8) = scaleRect(BottomRightRegion);
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
	auto &renderer = this->getGameState()->getRenderer();
	renderer.updateCamera(player.getPosition(), player.getDirection(), verticalFOV);
	renderer.updateGameTime(gameData->getGameTime());
}

void GameWorldPanel::render(Renderer &renderer)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Draw game world onto the native frame buffer. The game world buffer
	// might not completely fill up the native buffer (bottom corners), so 
	// clearing the native buffer beforehand is still necessary.
	renderer.renderWorld();

	// Set screen palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Set original frame buffer blending to true.
	renderer.useTransparencyBlending(true);

	// Draw game world interface.
	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));
	renderer.drawToOriginal(gameInterface.get(), 0, 
		Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight());

	// Draw player portrait.
	auto &player = this->getGameState()->getGameData()->getPlayer();
	const auto &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceName(), true);
	const auto &portrait = textureManager.getTextures(headsFilename)
		.at(player.getPortraitID());
	const auto &status = textureManager.getTextures(
		TextureFile::fromName(TextureName::StatusGradients)).at(0);
	renderer.drawToOriginal(status.get(), 14, 166);
	renderer.drawToOriginal(portrait.get(), 14, 166);

	// Draw compass slider (the actual headings). +X is north, +Z is east.
	// Should do some sin() and cos() functions to get the pixel offset.
	auto *compassSlider = textureManager.getSurface(
		TextureFile::fromName(TextureName::CompassSlider));

	Texture compassSliderSegment = [&renderer, &compassSlider]()
	{
		SDL_Surface *segmentTemp = Surface::createSurfaceWithFormat(32, 7,
			Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

		SDL_Rect clipRect;
		clipRect.x = 60; // Arbitrary offset until compass rotation works.
		clipRect.y = 0;
		clipRect.w = segmentTemp->w;
		clipRect.h = segmentTemp->h;

		SDL_BlitSurface(compassSlider, &clipRect, segmentTemp, nullptr);

		SDL_Texture *segment = renderer.createTextureFromSurface(segmentTemp);
		SDL_FreeSurface(segmentTemp);

		return Texture(segment);
	}();

	renderer.drawToOriginal(compassSliderSegment.get(),
		(Renderer::ORIGINAL_WIDTH / 2) - (compassSliderSegment.getWidth() / 2),
		compassSliderSegment.getHeight());

	// Draw compass frame over the headings.
	const auto &compassFrame = textureManager.getTexture(
		TextureFile::fromName(TextureName::CompassFrame));
	renderer.drawToOriginal(compassFrame.get(),
		(Renderer::ORIGINAL_WIDTH / 2) - (compassFrame.getWidth() / 2), 0);

	// Draw text: player name.
	renderer.drawToOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());

	// Scale the original frame buffer onto the native one.
	// This shouldn't be done for the game world interface because it needs to
	// clamp to the screen edges, not the letterbox edges. 
	// Fix this eventually... again.
	renderer.drawOriginalToNative();

	// Draw cursor, depending on its position on the screen.
	const Int2 mousePosition = this->getMousePosition();

	const Texture &cursor = [this, &mousePosition, &textureManager]()
		-> const Texture& // Interesting how this return type isn't deduced in MSVC.
	{
		// See which arrow cursor region the native mouse is in.
		for (int i = 0; i < this->nativeCursorRegions.size(); ++i)
		{
			if (this->nativeCursorRegions.at(i)->contains(mousePosition))
			{
				return textureManager.getTextures(
					TextureFile::fromName(TextureName::ArrowCursors)).at(i);
			}
		}

		// If not in any of the arrow regions, use the default sword cursor.
		return textureManager.getTexture(
			TextureFile::fromName(TextureName::SwordCursor));
	}();

	renderer.drawToNative(cursor.get(), mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));

	// Set the transparency blending back to normal (off).
	renderer.useTransparencyBlending(false);
}
