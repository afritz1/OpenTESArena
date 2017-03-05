#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "GameWorldPanel.h"

#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
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
}

enum class PlayerInterface
{
	Classic,
	Modern
};

GameWorldPanel::GameWorldPanel(Game *game)
	: Panel(game)
{
	assert(game->gameDataIsActive());

	this->playerNameTextBox = [game]()
	{
		int x = 17;
		int y = 154;
		Color color(215, 121, 8);
		std::string text = game->getGameData().getPlayer().getFirstName();
		auto &font = game->getFontManager().getFont(FontName::Char);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->characterSheetButton = []()
	{
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> sheetPanel(new CharacterPanel(game));
			game->setPanel(std::move(sheetPanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(14, 166, 40, 29, function));
	}();

	this->drawWeaponButton = []()
	{
		auto function = [](Game *game)
		{
			Debug::mention("Game", "Draw weapon.");
		};
		return std::unique_ptr<Button<>>(new Button<>(88, 151, 29, 22, function));
	}();

	this->stealButton = []()
	{
		auto function = [](Game *game)
		{
			Debug::mention("Game", "Steal.");
		};
		return std::unique_ptr<Button<>>(new Button<>(147, 151, 29, 22, function));
	}();

	this->statusButton = []()
	{
		auto function = [](Game *game)
		{
			Debug::mention("Game", "Status.");
		};
		return std::unique_ptr<Button<>>(new Button<>(177, 151, 29, 22, function));
	}();

	this->magicButton = []()
	{
		auto function = [](Game *game)
		{
			Debug::mention("Game", "Magic.");
		};
		return std::unique_ptr<Button<>>(new Button<>(88, 175, 29, 22, function));
	}();

	this->logbookButton = []()
	{
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> logbookPanel(new LogbookPanel(game));
			game->setPanel(std::move(logbookPanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(118, 175, 29, 22, function));
	}();

	this->useItemButton = []()
	{
		auto function = [](Game *game)
		{
			Debug::mention("Game", "Use item.");
		};
		return std::unique_ptr<Button<>>(new Button<>(147, 175, 29, 22, function));
	}();

	this->campButton = []()
	{
		auto function = [](Game *game)
		{
			Debug::mention("Game", "Camp.");
		};
		return std::unique_ptr<Button<>>(new Button<>(177, 175, 29, 22, function));
	}();

	this->scrollUpButton = []()
	{
		auto function = [](Game *game)
		{
			// Nothing yet.
		};

		// Y position is based on height of interface image.
		return std::unique_ptr<Button<>>(new Button<>(
			208, 
			(Renderer::ORIGINAL_HEIGHT - 53) + 3, 
			9, 
			9, 
			function));
	}();

	this->scrollDownButton = []()
	{
		auto function = [](Game *game)
		{
			// Nothing yet.
		};

		// Y position is based on height of interface image.
		return std::unique_ptr<Button<>>(new Button<>(
			208,
			(Renderer::ORIGINAL_HEIGHT - 53) + 44,
			9,
			9,
			function));
	}();

	this->pauseButton = []()
	{
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> pausePanel(new PauseMenuPanel(game));
			game->setPanel(std::move(pausePanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(function));
	}();

	this->mapButton = []()
	{
		auto function = [](Game *game, bool goToAutomap)
		{
			if (goToAutomap)
			{
				std::unique_ptr<Panel> automapPanel(new AutomapPanel(game));
				game->setPanel(std::move(automapPanel));
			}
			else
			{
				std::unique_ptr<Panel> worldMapPanel(new WorldMapPanel(game));
				game->setPanel(std::move(worldMapPanel));
			}
		};
		return std::unique_ptr<Button<bool>>(
			new Button<bool>(118, 151, 29, 22, function));
	}();

	// Default to classic mode for now. Eventually, "modern" mode will be
	// an option for a free-look camera like Daggerfall.
	this->playerInterface = PlayerInterface::Classic;

	// Set all of the cursor regions relative to the current window.
	const Int2 screenDims = game->getRenderer().getWindowDimensions();
	this->updateCursorRegions(screenDims.x, screenDims.y);
}

GameWorldPanel::~GameWorldPanel()
{

}

void GameWorldPanel::handleEvent(const SDL_Event &e)
{
	bool resized = (e.type == SDL_WINDOWEVENT) &&
		(e.window.event == SDL_WINDOWEVENT_RESIZED);

	if (resized)
	{
		// Update the cursor's regions for camera motion.
		int width = e.window.data1;
		int height = e.window.data2;
		this->updateCursorRegions(width, height);
	}

	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->pauseButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);
	bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_RIGHT);

	const auto &renderer = this->getGame()->getRenderer();

	// Get mouse position relative to letterbox coordinates.
	const Int2 originalPosition = renderer.nativePointToOriginal(
		this->getMousePosition());

	if (leftClick)
	{
		// Was an interface button clicked?
		if (this->characterSheetButton->contains(originalPosition))
		{
			this->characterSheetButton->click(this->getGame());
		}
		else if (this->drawWeaponButton->contains(originalPosition))
		{
			this->drawWeaponButton->click(this->getGame());
		}
		else if (this->mapButton->contains(originalPosition))
		{
			this->mapButton->click(this->getGame(), true);
		}
		else if (this->stealButton->contains(originalPosition))
		{
			this->stealButton->click(this->getGame());
		}
		else if (this->statusButton->contains(originalPosition))
		{
			this->statusButton->click(this->getGame());
		}
		else if (this->magicButton->contains(originalPosition))
		{
			this->magicButton->click(this->getGame());
		}
		else if (this->logbookButton->contains(originalPosition))
		{
			this->logbookButton->click(this->getGame());
		}
		else if (this->useItemButton->contains(originalPosition))
		{
			this->useItemButton->click(this->getGame());
		}
		else if (this->campButton->contains(originalPosition))
		{
			this->campButton->click(this->getGame());
		}

		// Later... any entities in the world clicked?
	}
	else if (rightClick)
	{
		if (this->mapButton->contains(originalPosition))
		{
			this->mapButton->click(this->getGame(), false);
		}
	}

	bool automapHotkeyPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_n);
	bool logbookHotkeyPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_l);
	bool sheetHotkeyPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_TAB);
	bool worldMapHotkeyPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_m);

	if (automapHotkeyPressed)
	{
		this->mapButton->click(this->getGame(), true);
	}
	else if (logbookHotkeyPressed)
	{
		this->logbookButton->click(this->getGame());
	}
	else if (sheetHotkeyPressed)
	{
		this->characterSheetButton->click(this->getGame());
	}
	else if (worldMapHotkeyPressed)
	{
		this->mapButton->click(this->getGame(), false);
	}
}

void GameWorldPanel::handlePlayerTurning(double dt)
{
	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicTurning()
	// 2) handleModernTurning()

	// Don't handle weapon swinging here. That can go in another method.
	// If right click is held, weapon is out, and mouse motion is significant, then
	// get the swing direction and swing.

	if (this->playerInterface == PlayerInterface::Classic)
	{
		// Classic interface mode.
		// Arena's mouse look is pretty clunky, and I much prefer the free-look model,
		// but this option needs to be here all the same.

		// Holding the LMB in the left, right, upper left, or upper right parts of the
		// screen turns the player. A and D turn the player as well.

		const auto &options = this->getGame()->getOptions();
		auto &player = this->getGame()->getGameData().getPlayer();

		// Listen for LMB, A, or D. Don't turn if Ctrl is held.
		const uint32_t mouse = SDL_GetMouseState(nullptr, nullptr);
		const bool leftClick = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

		const uint8_t *keys = SDL_GetKeyboardState(nullptr);
		bool left = keys[SDL_SCANCODE_A] != 0;
		bool right = keys[SDL_SCANCODE_D] != 0;
		bool lCtrl = keys[SDL_SCANCODE_LCTRL] != 0;
		
		// Mouse turning takes priority over key turning.
		if (leftClick)
		{
			const Int2 mousePosition = this->getMousePosition();

			// Strength of turning is determined by proximity of the mouse cursor to
			// the left or right screen edge.
			const double dx = [this, &mousePosition]()
			{
				const int mouseX = mousePosition.x;

				// Native cursor regions for turning (scaled to the current window).
				const Rect &topLeft = this->nativeCursorRegions.at(0);
				const Rect &topRight = this->nativeCursorRegions.at(2);
				const Rect &middleLeft = this->nativeCursorRegions.at(3);
				const Rect &middleRight = this->nativeCursorRegions.at(5);

				// Measure the magnitude of rotation. -1.0 is left, 1.0 is right.
				double percent = 0.0;
				if (topLeft.contains(mousePosition))
				{
					percent = -1.0 + (static_cast<double>(mouseX) / topLeft.getWidth());
				}
				else if (topRight.contains(mousePosition))
				{
					percent = static_cast<double>(mouseX - topRight.getLeft()) /
						topRight.getWidth();
				}
				else if (middleLeft.contains(mousePosition))
				{
					percent = -1.0 + (static_cast<double>(mouseX) / middleLeft.getWidth());
				}
				else if (middleRight.contains(mousePosition))
				{
					percent = static_cast<double>(mouseX - middleRight.getLeft()) /
						middleRight.getWidth();
				}

				// No NaNs or infinities allowed.
				return std::isfinite(percent) ? percent : 0.0;
			}();

			// Yaw the camera left or right. No vertical movement in classic camera mode.
			// Multiply turning speed by delta time so it behaves correctly with different
			// frame rates.
			player.rotate(dx * dt, 0.0, options.getHorizontalSensitivity(),
				options.getVerticalSensitivity());
		}
		else if (!lCtrl)
		{
			// If left control is not held, then turning is permitted.
			// Use an arbitrary turn speed mixed with the horizontal sensitivity.
			const double turnSpeed = 0.60;

			if (left)
			{
				// Turn left at a fixed angular velocity.
				player.rotate(-turnSpeed * dt, 0.0, options.getHorizontalSensitivity(),
					options.getVerticalSensitivity());
			}
			else if (right)
			{
				// Turn right at a fixed angular velocity.
				player.rotate(turnSpeed * dt, 0.0, options.getHorizontalSensitivity(),
					options.getVerticalSensitivity());
			}
		}
	}
	else
	{
		// Modern interface.
		Debug::crash("Game World", "Modern interface not implemented.");

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
			auto dimensions = this->getGame()->getRenderer().getWindowDimensions();
			double dxx = static_cast<double>(dx) / static_cast<double>(dimensions.x);
			double dyy = static_cast<double>(dy) / static_cast<double>(dimensions.y);

			// Pitch and/or yaw the camera.
			const auto &options = this->getGame()->getOptions();
			auto &player = this->getGame()->getGameData().getPlayer();
			player.rotate(dxx, -dyy, options.getHorizontalSensitivity(), 
				options.getVerticalSensitivity(), options.getVerticalFOV());
		}*/
	}
}

void GameWorldPanel::handlePlayerMovement(double dt)
{
	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicMovement()
	// 2) handleModernMovement()

	if (this->playerInterface == PlayerInterface::Classic)
	{
		// Classic interface mode.
		// Arena uses arrow keys, but let's use the left hand side of the keyboard 
		// because we like being comfortable.

		// A and D turn the player, and if Ctrl is held, the player slides instead.
		// Let's keep the turning part in the other method because turning doesn't
		// affect velocity.

		// Listen for mouse, WASD, and Ctrl.
		const uint32_t mouse = SDL_GetMouseState(nullptr, nullptr);
		const bool leftClick = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

		const uint8_t *keys = SDL_GetKeyboardState(nullptr);
		bool forward = keys[SDL_SCANCODE_W] != 0;
		bool backward = keys[SDL_SCANCODE_S] != 0;
		bool left = keys[SDL_SCANCODE_A] != 0;
		bool right = keys[SDL_SCANCODE_D] != 0;
		bool lCtrl = keys[SDL_SCANCODE_LCTRL] != 0;

		// The original game didn't have sprinting, but it seems like something 
		// relevant to do anyway (at least for development).
		bool isRunning = keys[SDL_SCANCODE_LSHIFT] != 0;

		// Arbitrary movement speeds.
		const double walkSpeed = 15.0;
		const double runSpeed = 30.0;

		auto &player = this->getGame()->getGameData().getPlayer();

		// -- test --
		// Some simple code to move the camera along the Y axis.
		bool space = keys[SDL_SCANCODE_SPACE] != 0;
		bool c = keys[SDL_SCANCODE_C] != 0;
		if (space)
		{
			player.teleport(player.getPosition() + Double3(0.0, 0.5 * dt, 0.0));
		}
		else if (c)
		{
			player.teleport(player.getPosition() - Double3(0.0, 0.5 * dt, 0.0));
		}
		// -- end test --

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0,
			groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();

		// Mouse movement takes priority over key movement.
		if (leftClick)
		{
			const Int2 mousePosition = this->getMousePosition();
			const int mouseX = mousePosition.x;
			const int mouseY = mousePosition.y;

			// Native cursor regions for motion (scaled to the current window).
			const Rect &topLeft = this->nativeCursorRegions.at(0);
			const Rect &top = this->nativeCursorRegions.at(1);
			const Rect &topRight = this->nativeCursorRegions.at(2);
			const Rect &bottomLeft = this->nativeCursorRegions.at(6);
			const Rect &bottom = this->nativeCursorRegions.at(7);
			const Rect &bottomRight = this->nativeCursorRegions.at(8);

			// Strength of movement is determined by the mouse's position in each region.
			// Motion magnitude (percent) is between 0.0 and 1.0.
			double percent = 0.0;
			Double3 accelDirection(0.0, 0.0, 0.0);
			if (topLeft.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection3D;
				percent = 1.0 - (static_cast<double>(mouseY) / topLeft.getHeight());
			}
			else if (top.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection3D;
				percent = 1.0 - (static_cast<double>(mouseY) / top.getHeight());
			}
			else if (topRight.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection3D;
				percent = 1.0 - (static_cast<double>(mouseY) / topRight.getHeight());
			}
			else if (bottomLeft.contains(mousePosition))
			{
				// Left.
				accelDirection = accelDirection - rightDirection;
				percent = 1.0 - (static_cast<double>(mouseX) / bottomLeft.getWidth());
			}
			else if (bottom.contains(mousePosition))
			{
				// Backwards.
				accelDirection = accelDirection - groundDirection3D;
				percent = static_cast<double>(mouseY - bottom.getTop()) /
					bottom.getHeight();
			}
			else if (bottomRight.contains(mousePosition))
			{
				// Right.
				accelDirection = accelDirection + rightDirection;
				percent = static_cast<double>(mouseX - bottomRight.getLeft()) /
					bottomRight.getWidth();
			}

			// Use a normalized direction.
			accelDirection = accelDirection.normalized();

			// Set the magnitude of the acceleration to some arbitrary number. These values 
			// are independent of max speed.
			double accelMagnitude = percent * (isRunning ? runSpeed : walkSpeed);

			// Change the player's velocity if valid.
			if (std::isfinite(accelDirection.length()) && 
				std::isfinite(accelMagnitude))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
		else if (forward || backward || ((left || right) && lCtrl))
		{
			// Calculate the acceleration direction based on input.
			Double3 accelDirection(0.0, 0.0, 0.0);
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

			// Set the magnitude of the acceleration to some arbitrary number. These values 
			// are independent of max speed.
			double accelMagnitude = isRunning ? runSpeed : walkSpeed;

			// Change the player's velocity if valid.
			if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
	}
	else
	{
		// Modern interface.
		Debug::crash("Game World", "Modern interface not implemented.");
	}
}

void GameWorldPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Font &font = this->getGame()->getFontManager().getFont(FontName::D);

	Texture tooltip(Panel::createTooltip(text, font, renderer));

	auto &textureManager = this->getGame()->getTextureManager();
	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));

	renderer.drawToOriginal(tooltip.get(), 0, Renderer::ORIGINAL_HEIGHT - 
		gameInterface.getHeight() - tooltip.getHeight());
}

void GameWorldPanel::drawDebugText(Renderer &renderer)
{
	const Int2 windowDims = renderer.getWindowDimensions();
	const double resolutionScale = this->getGame()->getOptions().getResolutionScale();

	const auto &player = this->getGame()->getGameData().getPlayer();
	const Double3 &position = player.getPosition();
	const Double3 &direction = player.getDirection();

	TextBox tempText(2, 2, Color::White,
		"Screen: " + std::to_string(windowDims.x) + "x" +
		std::to_string(windowDims.y) + "\n" +
		"Resolution scale: " + std::to_string(resolutionScale) + "\n" +
		"X: " + std::to_string(position.x) + "\n" +
		"Y: " + std::to_string(position.y) + "\n" +
		"Z: " + std::to_string(position.z) + "\n" +
		"DirX: " + std::to_string(direction.x) + "\n" +
		"DirY: " + std::to_string(direction.y) + "\n" +
		"DirZ: " + std::to_string(direction.z),
		this->getGame()->getFontManager().getFont(FontName::D),
		TextAlignment::Left, renderer);
	renderer.drawToOriginal(tempText.getTexture(), tempText.getX(), tempText.getY());
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

		return Rect(x, y, width, height);
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

void GameWorldPanel::tick(double dt)
{
	assert(this->getGame()->gameDataIsActive());

	// Handle input for player motion.
	this->handlePlayerTurning(dt);
	this->handlePlayerMovement(dt);

	// Animate the game world.
	// - Later, this should be more explicit about what it's updating exactly.
	// - Entity AI, spells flying, doors opening/closing, etc.
	auto &game = *this->getGame();
	auto &gameData = game.getGameData();
	gameData.incrementGameTime(dt);

	// Tick the player.
	auto &player = gameData.getPlayer();
	player.tick(game, dt);

	// Update renderer members that are refreshed each frame.
	auto &renderer = game.getRenderer();
	renderer.updateCamera(player.getPosition(), player.getDirection(), 
		this->getGame()->getOptions().getVerticalFOV());
	//renderer.updateGameTime(gameData.getGameTime()); // To do.
	renderer.updateFogDistance(gameData.getFogDistance());

	// Technically, the sky palette only needs to be updated when the scene changes.
	auto &textureManager = game.getTextureManager();
	const SDL_Surface *skyPalette = textureManager.getSurface("DAYTIME.COL");
	renderer.updateSkyPalette(static_cast<const uint32_t*>(skyPalette->pixels), 
		skyPalette->w * skyPalette->h);

	// -- test -- update test sprites.
	/*const Double3 playerRight = player.getFrame().getRight();
	const Float3 spriteRight(
		static_cast<float>(-playerRight.x),
		static_cast<float>(-playerRight.y),
		static_cast<float>(-playerRight.z));

	static int index = 0;
	for (int k = 1; k < gameData.getWorldDepth() - 1; k += 8)
	{
		for (int i = 1; i < gameData.getWorldWidth() - 1; i += 8)
		{
			Rect3D rect = Rect3D::fromFrame(
				Float3(i + 0.50f, 1.0f, k + 0.50f),
				spriteRight,
				Float3(0.0f, 1.0f, 0.0f),
				(44.0f / 66.0f) * 0.75f,
				1.0f * 0.75f);
			renderer.updateSprite(
				(i - 1) + ((k - 1) * (gameData.getWorldWidth() - 1)),
				rect,
				30 + ((index / 8) % 2));
		}
	}
	index++;*/
	// -- end test --
}

void GameWorldPanel::render(Renderer &renderer)
{
	assert(this->getGame()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Draw game world onto the native frame buffer. The game world buffer
	// might not completely fill up the native buffer (bottom corners), so 
	// clearing the native buffer beforehand is still necessary.
	auto &gameData = this->getGame()->getGameData();
	renderer.renderWorld(gameData.getVoxelGrid());

	// Set screen palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Set original frame buffer blending to true.
	renderer.useTransparencyBlending(true);

	// Draw some debug text.
	this->drawDebugText(renderer);

	// Draw game world interface.
	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));
	renderer.drawToOriginal(gameInterface.get(), 0,
		Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight());

	// Draw player portrait.
	const auto &player = this->getGame()->getGameData().getPlayer();
	const auto &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceName(), true);
	const auto &portrait = textureManager.getTextures(headsFilename)
		.at(player.getPortraitID());
	const auto &status = textureManager.getTextures(
		TextureFile::fromName(TextureName::StatusGradients)).at(0);
	renderer.drawToOriginal(status.get(), 14, 166);
	renderer.drawToOriginal(portrait.get(), 14, 166);

	// Draw compass slider based on player direction. +X is north, +Z is east.
	auto *compassSlider = textureManager.getSurface(
		TextureFile::fromName(TextureName::CompassSlider));
	const Double2 groundDirection = player.getGroundDirection();

	Texture compassSliderSegment = [&renderer, &compassSlider, &groundDirection]()
	{
		SDL_Surface *segmentTemp = Surface::createSurfaceWithFormat(32, 7,
			Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

		// Angle between 0 and 2 pi.
		const double angle = std::atan2(groundDirection.y, groundDirection.x);

		// Offset in the "slider" texture. Due to how SLIDER.IMG is drawn, there's a 
		// small "pop-in" when turning from N to NE, because N is drawn in two places, 
		// but the second place (offset == 256) has tick marks where "NE" should be.
		const int xOffset = static_cast<int>(240.0 + 
			std::round(256.0 * (angle / (2.0 * PI)))) % 256;

		SDL_Rect clipRect;
		clipRect.x = xOffset;
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

	// If the player's class can't use magic, show the darkened spell icon.
	if (!player.getCharacterClass().canCastMagic())
	{
		const auto &nonMagicIcon = textureManager.getTexture(
			TextureFile::fromName(TextureName::NoSpell));
		renderer.drawToOriginal(nonMagicIcon.get(), 91, 177);
	}

	// Draw text: player name.
	renderer.drawToOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());

	// Check if the mouse is over one of the buttons for tooltips.
	const Int2 originalPosition = renderer.nativePointToOriginal(this->getMousePosition());

	if (this->characterSheetButton->contains(originalPosition))
	{
		this->drawTooltip("Character Sheet", renderer);
	}
	else if (this->drawWeaponButton->contains(originalPosition))
	{
		this->drawTooltip("Draw/Sheathe Weapon", renderer);
	}
	else if (this->mapButton->contains(originalPosition))
	{
		this->drawTooltip("Automap/World Map", renderer);
	}
	else if (this->stealButton->contains(originalPosition))
	{
		this->drawTooltip("Steal", renderer);
	}
	else if (this->statusButton->contains(originalPosition))
	{
		this->drawTooltip("Status", renderer);
	}
	else if (this->magicButton->contains(originalPosition) &&
		player.getCharacterClass().canCastMagic())
	{
		this->drawTooltip("Cast Magic", renderer);
	}
	else if (this->logbookButton->contains(originalPosition))
	{
		this->drawTooltip("Logbook", renderer);
	}
	else if (this->useItemButton->contains(originalPosition))
	{
		this->drawTooltip("Use Item", renderer);
	}
	else if (this->campButton->contains(originalPosition))
	{
		this->drawTooltip("Camp", renderer);
	}

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
			if (this->nativeCursorRegions.at(i).contains(mousePosition))
			{
				return textureManager.getTextures(
					TextureFile::fromName(TextureName::ArrowCursors)).at(i);
			}
		}

	// If not in any of the arrow regions, use the default sword cursor.
	return textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	}();

	renderer.drawToNative(cursor.get(), mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));

	// Set the transparency blending back to normal (off).
	renderer.useTransparencyBlending(false);
}
