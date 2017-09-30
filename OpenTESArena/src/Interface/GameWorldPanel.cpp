#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "GameWorldPanel.h"

#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "WorldMapPanel.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/TextAssets.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Entity.h"
#include "../Entities/Player.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
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
#include "../Media/SoundFile.h"
#include "../Media/SoundName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

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

	// Arrow cursor alignments. These offset the drawn cursor relative to the mouse 
	// position so the cursor's click area is closer to the tip of each arrow, as is 
	// done in the original game (slightly differently, though. I think the middle 
	// cursor was originally top-aligned, not middle-aligned, which is strange).
	const std::array<CursorAlignment, 9> ArrowCursorAlignments =
	{
		CursorAlignment::TopLeft,
		CursorAlignment::Top,
		CursorAlignment::TopRight,
		CursorAlignment::TopLeft,
		CursorAlignment::Middle,
		CursorAlignment::TopRight,
		CursorAlignment::Left,
		CursorAlignment::Bottom,
		CursorAlignment::Right
	};
}

GameWorldPanel::GameWorldPanel(Game *game)
	: Panel(game), triggeredText(0.0, nullptr)
{
	assert(game->gameDataIsActive());

	this->playerNameTextBox = [game]()
	{
		const int x = 17;
		const int y = 154;

		const RichTextString richText(
			game->getGameData().getPlayer().getFirstName(),
			FontName::Char,
			Color(215, 121, 8),
			TextAlignment::Left,
			game->getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game->getRenderer()));
	}();

	this->characterSheetButton = []()
	{
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> sheetPanel(new CharacterPanel(game));
			game->setPanel(std::move(sheetPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(14, 166, 40, 29, function));
	}();

	this->drawWeaponButton = []()
	{
		auto function = [](Player &player)
		{
			WeaponAnimation &weaponAnimation = player.getWeaponAnimation();

			if (weaponAnimation.isSheathed())
			{
				// Begin unsheathing the weapon.
				weaponAnimation.setState(WeaponAnimation::State::Unsheathing);
			}
			else if (weaponAnimation.isIdle())
			{
				// Begin sheathing the weapon.
				weaponAnimation.setState(WeaponAnimation::State::Sheathing);
			}
		};
		return std::unique_ptr<Button<Player&>>(new Button<Player&>(88, 151, 29, 22, function));
	}();

	this->stealButton = []()
	{
		auto function = []()
		{
			DebugMention("Steal.");
		};
		return std::unique_ptr<Button<>>(new Button<>(147, 151, 29, 22, function));
	}();

	this->statusButton = []()
	{
		auto function = [](Game *game)
		{
			const auto playerInterface = game->getOptions().getPlayerInterface();

			// The center of the pop-up depends on the interface mode.
			const Int2 center = [game, playerInterface]()
			{
				auto &textureManager = game->getTextureManager();
				const auto &gameWorldInterface = textureManager.getTexture(
					TextureFile::fromName(TextureName::GameWorldInterface));

				if (playerInterface == PlayerInterface::Classic)
				{
					return Int2(
						Renderer::ORIGINAL_WIDTH / 2,
						(Renderer::ORIGINAL_HEIGHT - gameWorldInterface.getHeight()) / 2);
				}
				else
				{
					return Int2(
						Renderer::ORIGINAL_WIDTH / 2,
						Renderer::ORIGINAL_HEIGHT / 2);
				}
			}();

			const std::string text = [game]()
			{
				const Location &location = game->getGameData().getLocation();

				const std::string timeString = [game]()
				{
					const Clock &clock = game->getGameData().getClock();
					const int hours = clock.getHours12();
					const int minutes = clock.getMinutes();
					const std::string clockTimeString = std::to_string(hours) + ":" +
						((minutes < 10) ? "0" : "") + std::to_string(minutes);

					const int timeOfDayIndex = [game]()
					{
						// Arena has eight time ranges for each time of day. They aren't 
						// uniformly distributed -- midnight and noon are only one minute.
						const std::array<std::pair<Clock, int>, 8> clocksAndIndices =
						{
							std::make_pair(Clock::Midnight, 6),
							std::make_pair(Clock::Night1, 5),
							std::make_pair(Clock::EarlyMorning, 0),
							std::make_pair(Clock::Morning, 1),
							std::make_pair(Clock::Noon, 2),
							std::make_pair(Clock::Afternoon, 3),
							std::make_pair(Clock::Evening, 4),
							std::make_pair(Clock::Night2, 5)
						};

						const Clock &presentClock = game->getGameData().getClock();

						// Reverse iterate, checking which range the active one is in.
						for (auto it = clocksAndIndices.rbegin(); it != clocksAndIndices.rend(); ++it)
						{
							const Clock &clock = it->first;

							if (presentClock.getTotalSeconds() >= clock.getTotalSeconds())
							{
								return it->second;
							}
						}

						DebugCrash("No valid time of day.");
						return 0;
					}();

					const std::string &timeOfDayString = game->getTextAssets().getAExeStrings()
						.getList(ExeStringKey::TimesOfDay).at(timeOfDayIndex);

					return clockTimeString + " " + timeOfDayString;
				}();

				const auto &date = game->getGameData().getDate();
				const std::string &weekdayString = game->getTextAssets().getAExeStrings().getList(
					ExeStringKey::WeekdayNames).at(date.getWeekday());
				const std::string dayString = date.getOrdinalDay();
				const std::string &monthString = game->getTextAssets().getAExeStrings().getList(
					ExeStringKey::MonthNames).at(date.getMonth());
				const std::string yearString = std::to_string(date.getEra()) + "E " +
					std::to_string(date.getYear());

				return "You are in " + location.getName() + "." + "\n" +
					"It is " + timeString + "." + "\n" +
					"The date is " + weekdayString + ", " + dayString + " of " + monthString + 
					" in the year " + yearString + "\n" +
					"You are currently carrying 0 kg out of 0 kg." + "\n" +
					"You are healthy.";
			}();

			const Color color(251, 239, 77);
			const int lineSpacing = 1;

			const RichTextString richText(
				text,
				FontName::Arena,
				color,
				TextAlignment::Center,
				lineSpacing,
				game->getFontManager());

			const Int2 &richTextDimensions = richText.getDimensions();

			Texture texture(Texture::generate(Texture::PatternType::Dark, 
				richTextDimensions.x + 12, richTextDimensions.y + 12, 
				game->getTextureManager(), game->getRenderer()));

			const Int2 textureCenter = center;

			// The sub-panel does nothing after it's removed.
			auto function = [](Game *game) {};

			std::unique_ptr<Panel> textSubPanel(new TextSubPanel(
				game, center, richText, function, std::move(texture), textureCenter));

			game->pushSubPanel(std::move(textSubPanel));
		};
		return std::unique_ptr<Button<Game*>>(new Button<Game*>(177, 151, 29, 22, function));
	}();

	this->magicButton = []()
	{
		auto function = []()
		{
			DebugMention("Magic.");
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
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(118, 175, 29, 22, function));
	}();

	this->useItemButton = []()
	{
		auto function = []()
		{
			DebugMention("Use item.");
		};
		return std::unique_ptr<Button<>>(new Button<>(147, 175, 29, 22, function));
	}();

	this->campButton = []()
	{
		auto function = []()
		{
			DebugMention("Camp.");
		};
		return std::unique_ptr<Button<>>(new Button<>(177, 175, 29, 22, function));
	}();

	this->scrollUpButton = []()
	{
		auto function = [](GameWorldPanel *panel)
		{
			// Nothing yet.
		};

		// Y position is based on height of interface image.
		return std::unique_ptr<Button<GameWorldPanel*>>(new Button<GameWorldPanel*>(
			208,
			(Renderer::ORIGINAL_HEIGHT - 53) + 3,
			9,
			9,
			function));
	}();

	this->scrollDownButton = []()
	{
		auto function = [](GameWorldPanel *panel)
		{
			// Nothing yet.
		};

		// Y position is based on height of interface image.
		return std::unique_ptr<Button<GameWorldPanel*>>(new Button<GameWorldPanel*>(
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
		return std::unique_ptr<Button<Game*>>(new Button<Game*>(function));
	}();

	this->mapButton = []()
	{
		auto function = [](Game *game, bool goToAutomap)
		{
			if (goToAutomap)
			{
				auto &gameData = game->getGameData();
				const auto &player = gameData.getPlayer();
				const auto &voxelGrid = gameData.getWorldData().getVoxelGrid();
				const Location &location = gameData.getLocation();
				const Double3 &position = player.getPosition();

				std::unique_ptr<Panel> automapPanel(new AutomapPanel(game,
					Double2(position.x, position.z), player.getGroundDirection(), 
					voxelGrid, location.getName()));
				game->setPanel(std::move(automapPanel));
			}
			else
			{
				std::unique_ptr<Panel> worldMapPanel(new WorldMapPanel(game));
				game->setPanel(std::move(worldMapPanel));
			}
		};
		return std::unique_ptr<Button<Game*, bool>>(
			new Button<Game*, bool>(118, 151, 29, 22, function));
	}();

	// Set all of the cursor regions relative to the current window.
	const Int2 screenDims = game->getRenderer().getWindowDimensions();
	this->updateCursorRegions(screenDims.x, screenDims.y);

	// Load all the weapon offsets for the player's currently equipped weapon. If the
	// player can ever change weapons in-game (i.e., with a hotkey), then this will
	// need to be moved into update() instead.
	const auto &weaponAnimation = game->getGameData().getPlayer().getWeaponAnimation();
	const std::string weaponFilename = weaponAnimation.getAnimationFilename() + ".CIF";
	const CIFFile cifFile(weaponFilename, Palette());
	
	for (int i = 0; i < cifFile.getImageCount(); ++i)
	{
		this->weaponOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}
}

GameWorldPanel::~GameWorldPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> GameWorldPanel::getCurrentCursor() const
{
	// The cursor texture depends on the current mouse position.
	const auto &game = *this->getGame();
	auto &textureManager = game.getTextureManager();
	const auto playerInterface = game.getOptions().getPlayerInterface();
	const Int2 mousePosition = game.getInputManager().getMousePosition();

	// If using the modern interface, just use the default arrow cursor.
	if (playerInterface == PlayerInterface::Modern)
	{
		const auto &texture = textureManager.getTextures(
			TextureFile::fromName(TextureName::ArrowCursors)).at(4);
		return std::make_pair(texture.get(), CursorAlignment::Middle);
	}
	else
	{
		// See which arrow cursor region the native mouse is in.
		for (int i = 0; i < this->nativeCursorRegions.size(); ++i)
		{
			if (this->nativeCursorRegions.at(i).contains(mousePosition))
			{
				const auto &texture = textureManager.getTextures(
					TextureFile::fromName(TextureName::ArrowCursors)).at(i);
				return std::make_pair(texture.get(), ArrowCursorAlignments.at(i));
			}
		}

		// If not in any of the arrow regions, use the default sword cursor.
		const auto &texture = textureManager.getTexture(
			TextureFile::fromName(TextureName::SwordCursor));
		return std::make_pair(texture.get(), CursorAlignment::TopLeft);
	}
}

void GameWorldPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool f4Pressed = inputManager.keyPressed(e, SDLK_F4);

	if (escapePressed)
	{
		this->pauseButton->click(this->getGame());
	}
	else if (f4Pressed)
	{
		// Toggle debug display.
		auto &options = this->getGame()->getOptions();
		options.setShowDebug(!options.debugIsShown());
	}

	// Listen for hotkeys.
	bool automapHotkeyPressed = inputManager.keyPressed(e, SDLK_n);
	bool logbookHotkeyPressed = inputManager.keyPressed(e, SDLK_l);
	bool sheetHotkeyPressed = inputManager.keyPressed(e, SDLK_TAB);
	bool statusHotkeyPressed = inputManager.keyPressed(e, SDLK_v);
	bool worldMapHotkeyPressed = inputManager.keyPressed(e, SDLK_m);

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
	else if (statusHotkeyPressed)
	{
		this->statusButton->click(this->getGame());
	}
	else if (worldMapHotkeyPressed)
	{
		this->mapButton->click(this->getGame(), false);
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	const auto &renderer = this->getGame()->getRenderer();

	// Handle input events based on which player interface mode is active.
	const auto playerInterface = this->getGame()->getOptions().getPlayerInterface();
	if (playerInterface == PlayerInterface::Classic)
	{
		// Get mouse position relative to letterbox coordinates.
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);

		if (leftClick)
		{
			// Was an interface button clicked?
			if (this->characterSheetButton->contains(originalPosition))
			{
				this->characterSheetButton->click(this->getGame());
			}
			else if (this->drawWeaponButton->contains(originalPosition))
			{
				this->drawWeaponButton->click(this->getGame()->getGameData().getPlayer());
			}
			else if (this->mapButton->contains(originalPosition))
			{
				this->mapButton->click(this->getGame(), true);
			}
			else if (this->stealButton->contains(originalPosition))
			{
				this->stealButton->click();
			}
			else if (this->statusButton->contains(originalPosition))
			{
				this->statusButton->click(this->getGame());
			}
			else if (this->magicButton->contains(originalPosition))
			{
				this->magicButton->click();
			}
			else if (this->logbookButton->contains(originalPosition))
			{
				this->logbookButton->click(this->getGame());
			}
			else if (this->useItemButton->contains(originalPosition))
			{
				this->useItemButton->click();
			}
			else if (this->campButton->contains(originalPosition))
			{
				this->campButton->click();
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
	}
}

void GameWorldPanel::resize(int windowWidth, int windowHeight)
{
	// Update the cursor's regions for camera motion.
	this->updateCursorRegions(windowWidth, windowHeight);
}

void GameWorldPanel::handlePlayerTurning(double dt, const Int2 &mouseDelta)
{
	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicTurning()
	// 2) handleModernTurning()

	// Don't handle weapon swinging here. That can go in another method.
	// If right click is held, weapon is out, and mouse motion is significant, then
	// get the swing direction and swing.
	const auto &inputManager = this->getGame()->getInputManager();

	const auto playerInterface = this->getGame()->getOptions().getPlayerInterface();
	if (playerInterface == PlayerInterface::Classic)
	{
		// Classic interface mode.
		// Arena's mouse look is pretty clunky, and I much prefer the free-look model,
		// but this option needs to be here all the same.

		// Holding the LMB in the left, right, upper left, or upper right parts of the
		// screen turns the player. A and D turn the player as well.

		const auto &options = this->getGame()->getOptions();
		auto &player = this->getGame()->getGameData().getPlayer();

		// Listen for LMB, A, or D. Don't turn if Ctrl is held.
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

		bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// Mouse turning takes priority over key turning.
		if (leftClick)
		{
			const Int2 mousePosition = inputManager.getMousePosition();

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
		// Modern interface. Make the camera look around.
		// - Relative mouse state isn't called because it can only be called once per frame,
		//   and its value is used in multiple places.
		const int dx = mouseDelta.x;
		const int dy = mouseDelta.y;

		bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);
		bool turning = ((dx != 0) || (dy != 0)) && leftClick;

		if (turning)
		{
			const Int2 dimensions = this->getGame()->getRenderer().getWindowDimensions();

			// Get the smaller of the two dimensions, so the look sensitivity is relative 
			// to a square instead of a rectangle. This keeps the camera look independent 
			// of the aspect ratio.
			const int minDimension = std::min(dimensions.x, dimensions.y);

			double dxx = static_cast<double>(dx) / static_cast<double>(minDimension);
			double dyy = static_cast<double>(dy) / static_cast<double>(minDimension);

			// Pitch and/or yaw the camera.
			const auto &options = this->getGame()->getOptions();
			auto &player = this->getGame()->getGameData().getPlayer();
			player.rotate(dxx, -dyy, options.getHorizontalSensitivity(),
				options.getVerticalSensitivity());
		}
	}
}

void GameWorldPanel::handlePlayerMovement(double dt)
{
	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicMovement()
	// 2) handleModernMovement()

	const auto &inputManager = this->getGame()->getInputManager();

	// Arbitrary movement speeds.
	const double walkSpeed = 15.0;
	const double runSpeed = 30.0;

	const auto &worldData = this->getGame()->getGameData().getWorldData();

	const auto playerInterface = this->getGame()->getOptions().getPlayerInterface();
	if (playerInterface == PlayerInterface::Classic)
	{
		// Classic interface mode.
		// Arena uses arrow keys, but let's use the left hand side of the keyboard 
		// because we like being comfortable.

		// A and D turn the player, and if Ctrl is held, the player slides instead.
		// Let's keep the turning part in the other method because turning doesn't
		// affect velocity.

		// Listen for mouse, WASD, and Ctrl.
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

		bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		bool space = inputManager.keyIsDown(SDL_SCANCODE_SPACE);
		bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// The original game didn't have sprinting, but it seems like something 
		// relevant to do anyway (at least for development).
		bool isRunning = inputManager.keyIsDown(SDL_SCANCODE_LSHIFT);

		auto &player = this->getGame()->getGameData().getPlayer();

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0,
			groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();

		// Mouse movement takes priority over key movement.
		if (leftClick && player.onGround(worldData))
		{
			const Int2 mousePosition = inputManager.getMousePosition();
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

			// Check for jumping first (so the player can't slide jump on the first frame).
			const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);
			if (rightClick)
			{
				// Jump.
				player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
			}
			// Change the player's velocity if valid.
			else if (std::isfinite(accelDirection.length()) &&
				std::isfinite(accelMagnitude))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
		else if ((forward || backward || ((left || right) && lCtrl) || space) &&
			player.onGround(worldData))
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

			// Use a normalized direction.
			accelDirection = accelDirection.normalized();

			// Set the magnitude of the acceleration to some arbitrary number. These values 
			// are independent of max speed.
			double accelMagnitude = isRunning ? runSpeed : walkSpeed;

			// Check for jumping first (so the player can't slide jump on the first frame).
			if (space)
			{
				// Jump.
				player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
			}
			// Change the player's velocity if valid.
			else if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
	}
	else
	{
		// Modern interface. Listen for WASD.
		bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		bool space = inputManager.keyIsDown(SDL_SCANCODE_SPACE);

		// The original game didn't have sprinting, but it seems like something 
		// relevant to do anyway (at least for development).
		bool isRunning = inputManager.keyIsDown(SDL_SCANCODE_LSHIFT);

		auto &player = this->getGame()->getGameData().getPlayer();

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0,
			groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();

		if ((forward || backward || left || right || space) && player.onGround(worldData))
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

			// Use a normalized direction.
			accelDirection = accelDirection.normalized();

			// Set the magnitude of the acceleration to some arbitrary number. These values 
			// are independent of max speed.
			double accelMagnitude = isRunning ? runSpeed : walkSpeed;

			// Check for jumping first (so the player can't slide jump on the first frame).
			if (space)
			{
				// Jump.
				player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
			}
			// Change the player's horizontal velocity if valid.
			else if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
	}
}

void GameWorldPanel::handlePlayerAttack(const Int2 &mouseDelta)
{
	// To do: run this method at fixed time-steps instead of every frame, because if, 
	// for example, the game is running at 200 fps, then the player has to move their 
	// cursor much faster for it to count as a swing. The GameWorldPanel would probably 
	// need to save its own "swing" mouse delta independently of the input manager, or
	// maybe the game loop could call a "Panel::fixedTick()" method.

	// Only handle attacking if the player's weapon is currently idle.
	auto &weaponAnimation = this->getGame()->getGameData().getPlayer().getWeaponAnimation();
	if (weaponAnimation.isIdle())
	{
		const Int2 dimensions = this->getGame()->getRenderer().getWindowDimensions();

		// Get the smaller of the two dimensions, so the percentage change in mouse position 
		// is relative to a square instead of a rectangle.
		const int minDimension = std::min(dimensions.x, dimensions.y);

		// Percentages that the mouse moved across the screen.
		const double dxx = static_cast<double>(mouseDelta.x) / static_cast<double>(minDimension);
		const double dyy = static_cast<double>(mouseDelta.y) / static_cast<double>(minDimension);

		const auto &inputManager = this->getGame()->getInputManager();
		const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

		// If the mouse moves fast enough, it's considered an attack. The distances
		// are in percentages of screen dimensions.
		const double requiredDistance = 0.060;
		const double mouseDistance = std::sqrt((dxx * dxx) + (dyy * dyy));
		const bool isAttack = rightClick && (mouseDistance >= requiredDistance);

		if (isAttack)
		{
			// Convert the change in mouse coordinates to a vector. Reverse the change in
			// y so that positive values are up.
			const Double2 mouseDirection = Double2(dxx, -dyy).normalized();

			// Calculate the direction the mouse moved in (let's use cardinal directions
			// for convenience. Up means north (positive Y), right means east (positive X).
			// This could be refined in the future).
			CardinalDirectionName cardinalDirection = CardinalDirection::getDirectionName(
				Double2(mouseDirection.y, mouseDirection.x));

			// Set the weapon animation state.
			if (cardinalDirection == CardinalDirectionName::North)
			{
				weaponAnimation.setState(WeaponAnimation::State::Forward);
			}
			else if (cardinalDirection == CardinalDirectionName::NorthEast)
			{
				weaponAnimation.setState(WeaponAnimation::State::Right);
			}
			else if (cardinalDirection == CardinalDirectionName::East)
			{
				weaponAnimation.setState(WeaponAnimation::State::Right);
			}
			else if (cardinalDirection == CardinalDirectionName::SouthEast)
			{
				weaponAnimation.setState(WeaponAnimation::State::DownRight);
			}
			else if (cardinalDirection == CardinalDirectionName::South)
			{
				weaponAnimation.setState(WeaponAnimation::State::Down);
			}
			else if (cardinalDirection == CardinalDirectionName::SouthWest)
			{
				weaponAnimation.setState(WeaponAnimation::State::DownLeft);
			}
			else if (cardinalDirection == CardinalDirectionName::West)
			{
				weaponAnimation.setState(WeaponAnimation::State::Left);
			}
			else if (cardinalDirection == CardinalDirectionName::NorthWest)
			{
				weaponAnimation.setState(WeaponAnimation::State::Left);
			}

			// Play the swing sound.
			auto &audioManager = this->getGame()->getAudioManager();
			audioManager.playSound(SoundFile::fromName(SoundName::Swish));
		}
	}	
}

void GameWorldPanel::handleTriggers(const Int2 &voxel)
{
	auto &game = *this->getGame();
	auto &worldData = game.getGameData().getWorldData();

	// See if there's a text trigger.
	WorldData::TextTrigger *textTrigger = worldData.getTextTrigger(voxel);
	if (textTrigger != nullptr)
	{
		// Only display it if it should be displayed (i.e., not already displayed 
		// if it's a single-display text).
		const bool canDisplay = !textTrigger->isSingleDisplay() ||
			(textTrigger->isSingleDisplay() && !textTrigger->hasBeenDisplayed());

		if (canDisplay)
		{
			// Ignore the newline at the end.
			const std::string text = textTrigger->getText().substr(
				0, textTrigger->getText().size() - 1);
			const int lineSpacing = 1;

			const RichTextString richText(
				text,
				FontName::Arena,
				Color(215, 121, 8),
				TextAlignment::Center,
				lineSpacing,
				game.getFontManager());

			// Create the text box for display (set position to zero; the renderer will decide
			// where to draw it).
			const Color shadowColor(12, 12, 24);
			std::unique_ptr<TextBox> textBox(new TextBox(
				Int2(0, 0), 
				richText, 
				shadowColor,
				game.getRenderer()));

			// Assign the text box and its duration to the triggered text member. It will 
			// be displayed in the render method until the duration is no longer positive.
			const double duration = std::max(2.50, static_cast<double>(text.size()) * 0.050);
			this->triggeredText = std::make_pair(duration, std::move(textBox));

			// Set the text trigger as activated (regardless of whether or not it's single-shot,
			// just for consistency).
			textTrigger->setPreviouslyDisplayed(true);
		}
	}

	// See if there's a sound trigger.
	const std::string *soundTrigger = worldData.getSoundTrigger(voxel);
	if (soundTrigger != nullptr)
	{
		// Play the sound.
		auto &audioManager = game.getAudioManager();
		audioManager.playSound(*soundTrigger);
	}
}

void GameWorldPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip(Panel::createTooltip(
		text, FontName::D, this->getGame()->getFontManager(), renderer));

	auto &textureManager = this->getGame()->getTextureManager();
	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));

	renderer.drawToOriginal(tooltip.get(), 0, Renderer::ORIGINAL_HEIGHT -
		gameInterface.getHeight() - tooltip.getHeight());
}

void GameWorldPanel::drawDebugText(Renderer &renderer)
{
	const Int2 windowDims = renderer.getWindowDimensions();

	const auto &game = *this->getGame();
	const double resolutionScale = game.getOptions().getResolutionScale();

	auto &gameData = game.getGameData();
	const auto &player = gameData.getPlayer();
	const Double3 &position = player.getPosition();
	const Double3 &direction = player.getDirection();

	const int x = 2;
	const int y = 2;

	const std::string text =
		"Screen: " + std::to_string(windowDims.x) + "x" + std::to_string(windowDims.y) + "\n" +
		"Resolution scale: " + String::fixedPrecision(resolutionScale, 2) + "\n" +
		"FPS: " + String::fixedPrecision(game.getFPSCounter().getFPS(), 1) + "\n" +
		"X: " + String::fixedPrecision(position.x, 5) + "\n" +
		"Y: " + String::fixedPrecision(position.y, 5) + "\n" +
		"Z: " + String::fixedPrecision(position.z, 5) + "\n" +
		"DirX: " + String::fixedPrecision(direction.x, 5) + "\n" +
		"DirY: " + String::fixedPrecision(direction.y, 5) + "\n" +
		"DirZ: " + String::fixedPrecision(direction.z, 5);

	const RichTextString richText(
		text,
		FontName::D,
		Color::White,
		TextAlignment::Left,
		game.getFontManager());

	TextBox tempText(x, y, richText, renderer);

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

	// Get the relative mouse state (can only be called once per frame).	
	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mouseDelta = inputManager.getMouseDelta();

	// Handle input for player motion.
	this->handlePlayerTurning(dt, mouseDelta);
	this->handlePlayerMovement(dt);

	// Tick the game world clock time.
	auto &game = *this->getGame();
	auto &gameData = game.getGameData();
	gameData.tickTime(dt);

	// Tick the triggered text timer if the remaining duration is positive.
	if (this->triggeredText.first > 0.0)
	{
		this->triggeredText.first -= dt;
	}

	// Tick the player.
	auto &player = gameData.getPlayer();
	const Int3 oldPlayerVoxel = player.getVoxelPosition();
	player.tick(game, dt);
	const Int3 newPlayerVoxel = player.getVoxelPosition();

	// See if the player changed voxels in the XZ plane, and trigger text and sound 
	// events if so.
	if ((newPlayerVoxel.x != oldPlayerVoxel.x) ||
		(newPlayerVoxel.z != oldPlayerVoxel.z))
	{
		this->handleTriggers(Int2(newPlayerVoxel.x, newPlayerVoxel.z));
	}

	// Handle input for the player's attack.
	this->handlePlayerAttack(mouseDelta);

	auto &worldData = gameData.getWorldData();

	// Update entities and their state in the renderer.
	auto &entityManager = worldData.getEntityManager();
	for (auto *entity : entityManager.getAllEntities())
	{
		// Tick entity state.
		entity->tick(game, dt);

		// Update entity flat properties for rendering. Only update the flat's direction
		// if they face the player each frame (like a sprite).
		auto &renderer = game.getRenderer();
		const Double3 position = entity->getPosition();
		const Double2 direction = -player.getGroundDirection();
		const int textureID = entity->getTextureID();
		const bool flipped = entity->getFlipped();
		renderer.updateFlat(entity->getID(), &position,
			entity->facesPlayer() ? &direction : nullptr, nullptr, nullptr,
			&textureID, &flipped);
	}
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
	auto &player = gameData.getPlayer();
	const auto &worldData = gameData.getWorldData();
	const auto &options = this->getGame()->getOptions();
	renderer.renderWorld(player.getPosition(), player.getDirection(),
		options.getVerticalFOV(), gameData.getAmbientPercent(),
		gameData.getDaytimePercent(), worldData.getVoxelGrid());

	// Set screen palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Set original frame buffer blending to true.
	renderer.useTransparencyBlending(true);

	// Display player's weapon if unsheathed. The position also depends on whether
	// the interface is in classic or modern mode.
	const auto playerInterface = options.getPlayerInterface();
	const auto &weaponAnimation = player.getWeaponAnimation();
	if (!weaponAnimation.isSheathed())
	{
		const int index = weaponAnimation.getFrameIndex();
		const std::string weaponFilename = weaponAnimation.getAnimationFilename() + ".CIF";
		const Texture &weaponTexture = textureManager.getTextures(weaponFilename).at(index);
		const Int2 &weaponOffset = this->weaponOffsets.at(index);

		// Draw the current weapon image. Add 1 to the classic Y offset because Arena's
		// renderer has an off-by-one bug, and a 1 pixel gap appears in my renderer unless 
		// a small offset is added.
		renderer.drawToOriginal(weaponTexture.get(), weaponOffset.x, 
			(playerInterface == PlayerInterface::Classic) ? (weaponOffset.y + 1) : 
			(Renderer::ORIGINAL_HEIGHT - weaponTexture.getHeight()));
	}

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

	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();

	// Continue drawing more interface objects if in classic mode.
	if (playerInterface == PlayerInterface::Classic)
	{
		// Draw game world interface.
		const auto &gameInterface = textureManager.getTexture(
			TextureFile::fromName(TextureName::GameWorldInterface));
		renderer.drawToOriginal(gameInterface.get(), 0,
			Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight());

		// Draw player portrait.
		const auto &headsFilename = PortraitFile::getHeads(
			player.getGenderName(), player.getRaceID(), true);
		const auto &portrait = textureManager.getTextures(headsFilename)
			.at(player.getPortraitID());
		const auto &status = textureManager.getTextures(
			TextureFile::fromName(TextureName::StatusGradients)).at(0);
		renderer.drawToOriginal(status.get(), 14, 166);
		renderer.drawToOriginal(portrait.get(), 14, 166);

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
		const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);

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
			this->drawTooltip("Spells", renderer);
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
	}

	// Draw pop-up text if its duration is positive.
	// - To do: maybe give delta time to render()? Or store in tick()? I want to avoid 
	//   subtracting the time in tick() because it would always be one frame shorter then.
	if (this->triggeredText.first > 0.0)
	{
		const auto &gameInterface = textureManager.getTexture(
			TextureFile::fromName(TextureName::GameWorldInterface));

		const auto &triggeredTextBox = *this->triggeredText.second.get();
		const int centerX = (Renderer::ORIGINAL_WIDTH / 2) - (triggeredTextBox.getSurface()->w / 2);
		const int centerY = Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight() - 
			triggeredTextBox.getSurface()->h - 2;

		renderer.drawToOriginal(triggeredTextBox.getShadowTexture(), centerX - 1, centerY);
		renderer.drawToOriginal(triggeredTextBox.getTexture(), centerX, centerY);
	}

	// Draw some optional debug text.
	if (options.debugIsShown())
	{
		this->drawDebugText(renderer);
	}

	// Scale the original frame buffer onto the native one.
	// - This shouldn't be done for the game world interface because it needs to clamp 
	//   to the screen edges, not the letterbox edges. Fix this eventually... again.
	renderer.drawOriginalToNative();

	// Set the transparency blending back to normal (off).
	renderer.useTransparencyBlending(false);
}
