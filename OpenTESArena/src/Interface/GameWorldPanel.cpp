#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "WorldMapPanel.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/MiscAssets.h"
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
#include "../World/VoxelData.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelType.h"
#include "../World/WorldType.h"

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

	// Colors for UI text.
	const Color TriggerTextColor(215, 121, 8);
	const Color TriggerTextShadowColor(12, 12, 24);
	const Color ActionTextColor(195, 0, 0);
	const Color ActionTextShadowColor(12, 12, 24);
	const Color EffectTextColor(251, 239, 77);
	const Color EffectTextShadowColor(190, 113, 0);

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

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game)
{
	assert(game.gameDataIsActive());

	this->playerNameTextBox = [&game]()
	{
		const int x = 17;
		const int y = 154;

		const RichTextString richText(
			game.getGameData().getPlayer().getFirstName(),
			FontName::Char,
			Color(215, 121, 8),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->characterSheetButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<CharacterPanel>(game);
		};
		return Button<Game&>(14, 166, 40, 29, function);
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
		return Button<Player&>(88, 151, 29, 22, function);
	}();

	this->stealButton = []()
	{
		auto function = []()
		{
			DebugMention("Steal.");
		};
		return Button<>(147, 151, 29, 22, function);
	}();

	this->statusButton = []()
	{
		auto function = [](Game &game)
		{
			const bool modernInterface = game.getOptions().getModernInterface();

			// The center of the pop-up depends on the interface mode.
			const Int2 center = [&game, modernInterface]()
			{
				auto &textureManager = game.getTextureManager();
				const auto &gameWorldInterface = textureManager.getTexture(
					TextureFile::fromName(TextureName::GameWorldInterface));

				if (!modernInterface)
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

			const std::string text = [&game]()
			{
				const Location &location = game.getGameData().getLocation();

				const std::string timeString = [&game]()
				{
					const Clock &clock = game.getGameData().getClock();
					const int hours = clock.getHours12();
					const int minutes = clock.getMinutes();
					const std::string clockTimeString = std::to_string(hours) + ":" +
						((minutes < 10) ? "0" : "") + std::to_string(minutes);

					const int timeOfDayIndex = [&game]()
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

						const Clock &presentClock = game.getGameData().getClock();

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

					const std::string &timeOfDayString = game.getMiscAssets().getAExeStrings()
						.getList(ExeStringKey::TimesOfDay).at(timeOfDayIndex);

					return clockTimeString + " " + timeOfDayString;
				}();

				const auto &date = game.getGameData().getDate();
				const std::string &weekdayString = game.getMiscAssets().getAExeStrings().getList(
					ExeStringKey::WeekdayNames).at(date.getWeekday());
				const std::string dayString = date.getOrdinalDay();
				const std::string &monthString = game.getMiscAssets().getAExeStrings().getList(
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
				game.getFontManager());

			const Int2 &richTextDimensions = richText.getDimensions();

			Texture texture(Texture::generate(Texture::PatternType::Dark, 
				richTextDimensions.x + 12, richTextDimensions.y + 12, 
				game.getTextureManager(), game.getRenderer()));

			const Int2 textureCenter = center;

			auto function = [](Game &game)
			{
				game.popSubPanel();
			};

			game.pushSubPanel<TextSubPanel>(game, center, richText, function, 
				std::move(texture), textureCenter);
		};
		return Button<Game&>(177, 151, 29, 22, function);
	}();

	this->magicButton = []()
	{
		auto function = []()
		{
			DebugMention("Magic.");
		};
		return Button<>(88, 175, 29, 22, function);
	}();

	this->logbookButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<LogbookPanel>(game);
		};
		return Button<Game&>(118, 175, 29, 22, function);
	}();

	this->useItemButton = []()
	{
		auto function = []()
		{
			DebugMention("Use item.");
		};
		return Button<>(147, 175, 29, 22, function);
	}();

	this->campButton = []()
	{
		auto function = []()
		{
			DebugMention("Camp.");
		};
		return Button<>(177, 175, 29, 22, function);
	}();

	this->scrollUpButton = []()
	{
		// Y position is based on height of interface image.
		const int x = 208;
		const int y = (Renderer::ORIGINAL_HEIGHT - 53) + 3;

		auto function = [](GameWorldPanel &panel)
		{
			// Nothing yet.
		};

		return Button<GameWorldPanel&>(x, y, 9, 9, function);
	}();

	this->scrollDownButton = []()
	{
		// Y position is based on height of interface image.
		const int x = 208;
		const int y = (Renderer::ORIGINAL_HEIGHT - 53) + 44;

		auto function = [](GameWorldPanel &panel)
		{
			// Nothing yet.
		};
		return Button<GameWorldPanel&>(x, y, 9, 9, function);
	}();

	this->pauseButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<PauseMenuPanel>(game);
		};
		return Button<Game&>(function);
	}();

	this->mapButton = []()
	{
		auto function = [](Game &game, bool goToAutomap)
		{
			if (goToAutomap)
			{
				auto &gameData = game.getGameData();
				const auto &worldData = gameData.getWorldData();
				const auto &level = worldData.getLevels().at(worldData.getCurrentLevel());
				const auto &player = gameData.getPlayer();
				const Location &location = gameData.getLocation();
				const Double3 &position = player.getPosition();

				game.setPanel<AutomapPanel>(game, Double2(position.x, position.z), 
					player.getGroundDirection(), level.getVoxelGrid(), location.getName());
			}
			else
			{
				game.setPanel<WorldMapPanel>(game);
			}
		};
		return Button<Game&, bool>(118, 151, 29, 22, function);
	}();

	// Set all of the cursor regions relative to the current window.
	const Int2 screenDims = game.getRenderer().getWindowDimensions();
	this->updateCursorRegions(screenDims.x, screenDims.y);

	// Load all the weapon offsets for the player's currently equipped weapon. If the
	// player can ever change weapons in-game (i.e., with a hotkey), then this will
	// need to be moved into update() instead.
	const auto &weaponAnimation = game.getGameData().getPlayer().getWeaponAnimation();
	const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
	const CIFFile cifFile(weaponFilename, Palette());
	
	for (int i = 0; i < cifFile.getImageCount(); i++)
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
	const auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const bool modernInterface = game.getOptions().getModernInterface();
	const Int2 mousePosition = game.getInputManager().getMousePosition();

	// If using the modern interface, just use the default arrow cursor.
	if (modernInterface)
	{
		const auto &texture = textureManager.getTextures(
			TextureFile::fromName(TextureName::ArrowCursors)).at(4);
		return std::make_pair(texture.get(), CursorAlignment::Middle);
	}
	else
	{
		// See which arrow cursor region the native mouse is in.
		for (int i = 0; i < this->nativeCursorRegions.size(); i++)
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
	auto &game = this->getGame();
	auto &options = game.getOptions();
	auto &player = game.getGameData().getPlayer();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool f4Pressed = inputManager.keyPressed(e, SDLK_F4);

	if (escapePressed)
	{
		this->pauseButton.click(game);
	}
	else if (f4Pressed)
	{
		// Toggle debug display.
		options.setShowDebug(!options.getShowDebug());
	}

	// Listen for hotkeys.
	bool drawWeaponHotkeyPressed = inputManager.keyPressed(e, SDLK_f);
	bool automapHotkeyPressed = inputManager.keyPressed(e, SDLK_n);
	bool logbookHotkeyPressed = inputManager.keyPressed(e, SDLK_l);
	bool sheetHotkeyPressed = inputManager.keyPressed(e, SDLK_TAB) ||
		inputManager.keyPressed(e, SDLK_F1);
	bool statusHotkeyPressed = inputManager.keyPressed(e, SDLK_v);
	bool worldMapHotkeyPressed = inputManager.keyPressed(e, SDLK_m);
	bool toggleCompassHotkeyPressed = inputManager.keyPressed(e, SDLK_F8);

	if (drawWeaponHotkeyPressed)
	{
		this->drawWeaponButton.click(player);
	}
	else if (automapHotkeyPressed)
	{
		this->mapButton.click(game, true);
	}
	else if (logbookHotkeyPressed)
	{
		this->logbookButton.click(game);
	}
	else if (sheetHotkeyPressed)
	{
		this->characterSheetButton.click(game);
	}
	else if (statusHotkeyPressed)
	{
		this->statusButton.click(game);
	}
	else if (worldMapHotkeyPressed)
	{
		this->mapButton.click(game, false);
	}
	else if (toggleCompassHotkeyPressed)
	{
		// Toggle compass display.
		options.setShowCompass(!options.getShowCompass());
	}

	// Player's XY coordinate hotkey.
	const bool f2Pressed = inputManager.keyPressed(e, SDLK_F2);

	if (f2Pressed)
	{
		// Refresh player coordinates display (probably intended for debugging in the
		// original game). These coordinates are in Arena's coordinate system.
		const auto &worldData = game.getGameData().getWorldData();
		const auto &voxelGrid = worldData.getLevels().at(0).getVoxelGrid();
		const Int2 originalVoxel = VoxelGrid::getTransformedCoordinate(
			Int2(player.getVoxelPosition().x, player.getVoxelPosition().z),
			voxelGrid.getWidth(), voxelGrid.getDepth());

		const std::string text = "Your position is " + std::to_string(originalVoxel.x) +
			", " + std::to_string(originalVoxel.y) + ".";

		const RichTextString richText(
			text,
			FontName::Arena,
			ActionTextColor,
			TextAlignment::Center,
			game.getFontManager());

		const TextBox::ShadowData shadowData(ActionTextShadowColor, Int2(-1, 0));

		// Create the text box for display (set position to zero; the renderer will decide
		// where to draw it).
		std::unique_ptr<TextBox> textBox(new TextBox(
			Int2(0, 0),
			richText,
			&shadowData,
			game.getRenderer()));

		// Assign the text box and its duration to the action text.
		auto &gameData = game.getGameData();
		auto &actionText = gameData.getActionText();
		const double duration = std::max(2.25, static_cast<double>(text.size()) * 0.050);
		actionText = std::make_pair(duration, std::move(textBox));
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	const auto &renderer = game.getRenderer();

	// Handle input events based on which player interface mode is active.
	const bool modernInterface = game.getOptions().getModernInterface();
	if (!modernInterface)
	{
		// Get mouse position relative to letterbox coordinates.
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

		if (leftClick)
		{
			// Was an interface button clicked?
			if (this->characterSheetButton.contains(originalPosition))
			{
				this->characterSheetButton.click(game);
			}
			else if (this->drawWeaponButton.contains(originalPosition))
			{
				this->drawWeaponButton.click(player);
			}
			else if (this->mapButton.contains(originalPosition))
			{
				this->mapButton.click(game, true);
			}
			else if (this->stealButton.contains(originalPosition))
			{
				this->stealButton.click();
			}
			else if (this->statusButton.contains(originalPosition))
			{
				this->statusButton.click(game);
			}
			else if (this->magicButton.contains(originalPosition))
			{
				this->magicButton.click();
			}
			else if (this->logbookButton.contains(originalPosition))
			{
				this->logbookButton.click(game);
			}
			else if (this->useItemButton.contains(originalPosition))
			{
				this->useItemButton.click();
			}
			else if (this->campButton.contains(originalPosition))
			{
				this->campButton.click();
			}

			// Later... any entities in the world clicked?
		}
		else if (rightClick)
		{
			if (this->mapButton.contains(originalPosition))
			{
				this->mapButton.click(game, false);
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
	const auto &inputManager = this->getGame().getInputManager();

	const bool modernInterface = this->getGame().getOptions().getModernInterface();
	if (!modernInterface)
	{
		// Classic interface mode.
		// Arena's mouse look is pretty clunky, and I much prefer the free-look model,
		// but this option needs to be here all the same.

		// Holding the LMB in the left, right, upper left, or upper right parts of the
		// screen turns the player. A and D turn the player as well.

		const auto &options = this->getGame().getOptions();
		auto &player = this->getGame().getGameData().getPlayer();

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
			const Int2 dimensions = this->getGame().getRenderer().getWindowDimensions();

			// Get the smaller of the two dimensions, so the look sensitivity is relative 
			// to a square instead of a rectangle. This keeps the camera look independent 
			// of the aspect ratio.
			const int minDimension = std::min(dimensions.x, dimensions.y);

			double dxx = static_cast<double>(dx) / static_cast<double>(minDimension);
			double dyy = static_cast<double>(dy) / static_cast<double>(minDimension);

			// Pitch and/or yaw the camera.
			const auto &options = this->getGame().getOptions();
			auto &player = this->getGame().getGameData().getPlayer();
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

	const auto &inputManager = this->getGame().getInputManager();

	// Arbitrary movement speeds.
	const double walkSpeed = 15.0;
	const double runSpeed = 30.0;

	const auto &worldData = this->getGame().getGameData().getWorldData();

	const bool modernInterface = this->getGame().getOptions().getModernInterface();
	if (!modernInterface)
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

		auto &player = this->getGame().getGameData().getPlayer();

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

		auto &player = this->getGame().getGameData().getPlayer();

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
	auto &weaponAnimation = this->getGame().getGameData().getPlayer().getWeaponAnimation();
	if (weaponAnimation.isIdle())
	{
		const Int2 dimensions = this->getGame().getRenderer().getWindowDimensions();

		// Get the smaller of the two dimensions, so the percentage change in mouse position 
		// is relative to a square instead of a rectangle.
		const int minDimension = std::min(dimensions.x, dimensions.y);

		// Percentages that the mouse moved across the screen.
		const double dxx = static_cast<double>(mouseDelta.x) / static_cast<double>(minDimension);
		const double dyy = static_cast<double>(mouseDelta.y) / static_cast<double>(minDimension);

		const auto &inputManager = this->getGame().getInputManager();
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
			auto &audioManager = this->getGame().getAudioManager();
			audioManager.playSound(SoundFile::fromName(SoundName::Swish));
		}
	}	
}

void GameWorldPanel::handleTriggers(const Int2 &voxel)
{
	auto &game = this->getGame();
	auto &worldData = game.getGameData().getWorldData();
	auto &level = worldData.getLevels().at(worldData.getCurrentLevel());

	// See if there's a text trigger.
	LevelData::TextTrigger *textTrigger = level.getTextTrigger(voxel);
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
				TriggerTextColor,
				TextAlignment::Center,
				lineSpacing,
				game.getFontManager());

			const TextBox::ShadowData shadowData(TriggerTextShadowColor, Int2(-1, 0));

			// Create the text box for display (set position to zero; the renderer will decide
			// where to draw it).
			std::unique_ptr<TextBox> textBox(new TextBox(
				Int2(0, 0), 
				richText, 
				&shadowData,
				game.getRenderer()));

			// Assign the text box and its duration to the triggered text member. It will 
			// be displayed in the render method until the duration is no longer positive.
			auto &gameData = game.getGameData();
			auto &triggerText = gameData.getTriggerText();
			const double duration = std::max(2.50, static_cast<double>(text.size()) * 0.050);
			triggerText = std::make_pair(duration, std::move(textBox));

			// Set the text trigger as activated (regardless of whether or not it's single-shot,
			// just for consistency).
			textTrigger->setPreviouslyDisplayed(true);
		}
	}

	// See if there's a sound trigger.
	const std::string *soundTrigger = level.getSoundTrigger(voxel);
	if (soundTrigger != nullptr)
	{
		// Play the sound.
		auto &audioManager = game.getAudioManager();
		audioManager.playSound(*soundTrigger);
	}
}

void GameWorldPanel::handleLevelTransition(const Int2 &playerVoxel, const Int2 &transitionVoxel)
{
	auto &game = this->getGame();
	auto &worldData = game.getGameData().getWorldData();
	const auto &level = worldData.getLevels().at(worldData.getCurrentLevel());
	const auto &voxelGrid = level.getVoxelGrid();

	// Get the voxel data associated with the voxel.
	const auto &voxelData = [&transitionVoxel, &voxelGrid]()
	{
		const uint8_t voxelID = [&transitionVoxel, &voxelGrid]()
		{
			const int x = transitionVoxel.x;
			const int y = 1;
			const int z = transitionVoxel.y;
			return voxelGrid.getVoxels()[x + (y * voxelGrid.getWidth()) +
				(z * voxelGrid.getWidth() * voxelGrid.getHeight())];
		}();

		return voxelGrid.getVoxelData(voxelID);
	}();

	// If the associated voxel data is a wall, then it might be a transition voxel.
	if (voxelData.dataType == VoxelDataType::Wall)
	{
		const VoxelData::WallData &wallData = voxelData.wall;
		auto &player = game.getGameData().getPlayer();

		// The direction from a level up/down voxel to where the player should end up after
		// going through. In other words, it points to the destination voxel adjacent to the
		// level up/down voxel.
		auto dirToNewVoxel = [&playerVoxel, &transitionVoxel]()
		{
			const int diffX = transitionVoxel.x - playerVoxel.x;
			const int diffZ = transitionVoxel.y - playerVoxel.y;

			// To do: this probably isn't robust enough. Maybe also check the player's angle
			// of velocity with angles to the voxel's corners to get the "arrival vector"
			// and thus the "near face" that is intersected, because this method doesn't
			// handle the player coming in at a diagonal.

			// Check which way the player is going and get the reverse of it.
			if (diffX > 0)
			{
				// From south to north.
				return -Double3::UnitX;
			}
			else if (diffX < 0)
			{
				// From north to south.
				return Double3::UnitX;
			}
			else if (diffZ > 0)
			{
				// From west to east.
				return -Double3::UnitZ;
			}
			else if (diffZ < 0)
			{
				// From east to west.
				return Double3::UnitZ;
			}
			else
			{
				throw std::runtime_error("Bad player transition voxel.");
			}
		}();

		// Player destination after going through a level up/down voxel.
		const Double3 destinationPoint(
			(static_cast<double>(transitionVoxel.x) + 0.50) + dirToNewVoxel.x,
			player.getPosition().y,
			(static_cast<double>(transitionVoxel.y) + 0.50) + dirToNewVoxel.z);

		// Check the voxel type to determine what it is exactly.
		if ((voxelData.type == VoxelType::Menu) &&
			(wallData.menuID != VoxelData::WallData::NO_MENU))
		{
			DebugMention("Entered *MENU " + std::to_string(wallData.menuID) + ".");
		}
		else if (voxelData.type == VoxelType::LevelUp)
		{
			if (worldData.getCurrentLevel() > 0)
			{
				worldData.setLevelActive(worldData.getCurrentLevel() - 1,
					game.getTextureManager(), game.getRenderer());

				player.teleport(destinationPoint);
				player.lookAt(player.getPosition() + dirToNewVoxel);
				player.setVelocityToZero();
			}
		}
		else if (voxelData.type == VoxelType::LevelDown)
		{
			worldData.setLevelActive(worldData.getCurrentLevel() + 1,
				game.getTextureManager(), game.getRenderer());

			player.teleport(destinationPoint);
			player.lookAt(player.getPosition() + dirToNewVoxel);
			player.setVelocityToZero();
		}
	}
}

void GameWorldPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip(Panel::createTooltip(
		text, FontName::D, this->getGame().getFontManager(), renderer));

	auto &textureManager = this->getGame().getTextureManager();
	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));

	renderer.drawOriginal(tooltip.get(), 0, Renderer::ORIGINAL_HEIGHT -
		gameInterface.getHeight() - tooltip.getHeight());
}

void GameWorldPanel::drawCompass(const Double2 &direction, 
	TextureManager &textureManager, Renderer &renderer)
{
	// Draw compass slider based on player direction. +X is north, +Z is east.
	const auto &compassSlider = textureManager.getTexture(
		TextureFile::fromName(TextureName::CompassSlider));

	// Angle between 0 and 2 pi.
	const double angle = std::atan2(direction.y, direction.x);

	// Offset in the "slider" texture. Due to how SLIDER.IMG is drawn, there's a 
	// small "pop-in" when turning from N to NE, because N is drawn in two places, 
	// but the second place (offset == 256) has tick marks where "NE" should be.
	const int xOffset = static_cast<int>(240.0 +
		std::round(256.0 * (angle / (2.0 * Constants::Pi)))) % 256;

	// Clip area for the visible part of the slider.
	const Rect clipRect(xOffset, 0, 32, compassSlider.getHeight());

	// Top-left corner of the slider in 320x200 space.
	const int sliderX = (Renderer::ORIGINAL_WIDTH / 2) - (clipRect.getWidth() / 2);
	const int sliderY = clipRect.getHeight();

	// Since there are some off-by-one rounding errors with SDL_RenderCopy,
	// draw a black rectangle behind the slider to cover up gaps.
	renderer.fillOriginalRect(Color::Black, sliderX - 1, sliderY - 1,
		clipRect.getWidth() + 2, clipRect.getHeight() + 2);

	renderer.drawOriginalClipped(compassSlider.get(), clipRect, sliderX, sliderY);

	// Draw the compass frame over the slider.
	const auto &compassFrame = textureManager.getTexture(
		TextureFile::fromName(TextureName::CompassFrame));
	renderer.drawOriginal(compassFrame.get(),
		(Renderer::ORIGINAL_WIDTH / 2) - (compassFrame.getWidth() / 2), 0);
}

void GameWorldPanel::drawDebugText(Renderer &renderer)
{
	const Int2 windowDims = renderer.getWindowDimensions();

	const auto &game = this->getGame();
	const double resolutionScale = game.getOptions().getResolutionScale();

	auto &gameData = game.getGameData();
	const auto &player = gameData.getPlayer();
	const Double3 &position = player.getPosition();
	const Double3 &direction = player.getDirection();

	const auto &worldData = gameData.getWorldData();
	const auto &level = worldData.getLevels().at(worldData.getCurrentLevel());

	const std::string text =
		"Screen: " + std::to_string(windowDims.x) + "x" + std::to_string(windowDims.y) + "\n" +
		"Resolution scale: " + String::fixedPrecision(resolutionScale, 2) + "\n" +
		"FPS: " + String::fixedPrecision(game.getFPSCounter().getFPS(), 1) + "\n" +
		"Map: " + worldData.getMifName() + "\n" +
		"Info: " + level.getInfName() + "\n" +
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

	const int x = 2;
	const int y = 2;
	const TextBox tempText(x, y, richText, renderer);

	renderer.drawOriginal(tempText.getTexture(), tempText.getX(), tempText.getY());
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
	assert(this->getGame().gameDataIsActive());

	// Get the relative mouse state (can only be called once per frame).	
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mouseDelta = inputManager.getMouseDelta();

	// Handle input for player motion.
	this->handlePlayerTurning(dt, mouseDelta);
	this->handlePlayerMovement(dt);

	// Tick the game world clock time.
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	const Clock oldClock = gameData.getClock();
	gameData.tickTime(dt);
	const Clock newClock = gameData.getClock();

	auto &renderer = game.getRenderer();

	// See if the clock passed the boundary between night and day, and vice versa.
	const bool activateNightLights = 
		(oldClock.getPreciseTotalSeconds() < Clock::LamppostActivate.getPreciseTotalSeconds()) &&
		(newClock.getPreciseTotalSeconds() >= Clock::LamppostActivate.getPreciseTotalSeconds());
	const bool deactivateNightLights = 
		(oldClock.getPreciseTotalSeconds() < Clock::LamppostDeactivate.getPreciseTotalSeconds()) &&
		(newClock.getPreciseTotalSeconds() >= Clock::LamppostDeactivate.getPreciseTotalSeconds());

	if (activateNightLights)
	{
		renderer.setNightLightsActive(true);
	}
	else if (deactivateNightLights)
	{
		renderer.setNightLightsActive(false);
	}

	// Tick text timers if their remaining duration is positive.
	auto &triggerText = gameData.getTriggerText();
	auto &actionText = gameData.getActionText();

	if (triggerText.first > 0.0)
	{
		triggerText.first -= dt;
	}

	if (actionText.first > 0.0)
	{
		actionText.first -= dt;
	}

	// To do: tick effect text, and draw in render().

	// Tick the player.
	auto &player = gameData.getPlayer();
	const Int3 oldPlayerVoxel = player.getVoxelPosition();
	player.tick(game, dt);
	const Int3 newPlayerVoxel = player.getVoxelPosition();

	// Handle input for the player's attack.
	this->handlePlayerAttack(mouseDelta);

	auto &worldData = gameData.getWorldData();

	// Update entities and their state in the renderer.
	auto &entityManager = worldData.getEntityManager();
	for (auto *entity : entityManager.getAllEntities())
	{
		// Tick entity state.
		entity->tick(game, dt);

		// Update entity flat properties for rendering.
		const Double3 position = entity->getPosition();
		const int textureID = entity->getTextureID();
		const bool flipped = entity->getFlipped();
		renderer.updateFlat(entity->getID(), &position, nullptr, nullptr,
			&textureID, &flipped);
	}

	// See if the player changed voxels in the XZ plane. If so, trigger text and
	// sound events, and handle any level transition.
	if ((newPlayerVoxel.x != oldPlayerVoxel.x) ||
		(newPlayerVoxel.z != oldPlayerVoxel.z))
	{
		const Int2 oldPlayerVoxelXZ(oldPlayerVoxel.x, oldPlayerVoxel.z);
		const Int2 newPlayerVoxelXZ(newPlayerVoxel.x, newPlayerVoxel.z);

		this->handleTriggers(newPlayerVoxelXZ);

		// To do: determine if the player would collide with the voxel instead
		// of checking that they're in the voxel.
		this->handleLevelTransition(oldPlayerVoxelXZ, newPlayerVoxelXZ);
	}
}

void GameWorldPanel::render(Renderer &renderer)
{
	assert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Draw game world onto the native frame buffer. The game world buffer
	// might not completely fill up the native buffer (bottom corners), so 
	// clearing the native buffer beforehand is still necessary.
	auto &gameData = this->getGame().getGameData();
	auto &player = gameData.getPlayer();
	const auto &worldData = gameData.getWorldData();
	const auto &level = worldData.getLevels().at(worldData.getCurrentLevel());
	const auto &options = this->getGame().getOptions();
	const double ambientPercent = [&gameData, &worldData]()
	{
		// Interiors are always completely dark, but for testing purposes, they
		// will be 100% bright until lights are implemented.
		// To do: take into account "outdoorDungeon". Add it to LevelData?
		if (worldData.getWorldType() == WorldType::Interior)
		{
			return 1.0;
		}
		else
		{
			return gameData.getAmbientPercent();
		}
	}();

	renderer.renderWorld(player.getPosition(), player.getDirection(),
		options.getVerticalFOV(), ambientPercent, gameData.getDaytimePercent(), 
		level.getCeilingHeight(), level.getVoxelGrid());

	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const bool modernInterface = options.getModernInterface();

	// Continue drawing more interface objects if in classic mode.
	// - To do: clamp game world interface to screen edges, not letterbox edges.
	if (!modernInterface)
	{
		// Draw game world interface.
		const auto &gameInterface = textureManager.getTexture(
			TextureFile::fromName(TextureName::GameWorldInterface));
		renderer.drawOriginal(gameInterface.get(), 0,
			Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight());

		// Draw player portrait.
		const auto &headsFilename = PortraitFile::getHeads(
			player.getGenderName(), player.getRaceID(), true);
		const auto &portrait = textureManager.getTextures(headsFilename)
			.at(player.getPortraitID());
		const auto &status = textureManager.getTextures(
			TextureFile::fromName(TextureName::StatusGradients)).at(0);
		renderer.drawOriginal(status.get(), 14, 166);
		renderer.drawOriginal(portrait.get(), 14, 166);

		// If the player's class can't use magic, show the darkened spell icon.
		if (!player.getCharacterClass().canCastMagic())
		{
			const auto &nonMagicIcon = textureManager.getTexture(
				TextureFile::fromName(TextureName::NoSpell));
			renderer.drawOriginal(nonMagicIcon.get(), 91, 177);
		}

		// Draw text: player name.
		renderer.drawOriginal(this->playerNameTextBox->getTexture(),
			this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	}

	// Draw some optional debug text.
	if (options.getShowDebug())
	{
		this->drawDebugText(renderer);
	}
}

void GameWorldPanel::renderSecondary(Renderer &renderer)
{
	assert(this->getGame().gameDataIsActive());

	// Several interface objects are in this method because they are hidden by the status
	// pop-up and the spells list.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));

	auto &gameData = this->getGame().getGameData();
	auto &player = gameData.getPlayer();
	const auto &options = this->getGame().getOptions();
	const bool modernInterface = options.getModernInterface();

	// Display player's weapon if unsheathed. The position also depends on whether
	// the interface is in classic or modern mode.
	const auto &weaponAnimation = player.getWeaponAnimation();
	if (!weaponAnimation.isSheathed())
	{
		const int index = weaponAnimation.getFrameIndex();
		const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
		const Texture &weaponTexture = textureManager.getTextures(weaponFilename).at(index);
		const Int2 &weaponOffset = this->weaponOffsets.at(index);

		// Draw the current weapon image depending on interface mode.
		if (modernInterface)
		{
			// Scale vertically to fit, then scale/translate everything else accordingly.
			const double weaponScale = static_cast<double>(Renderer::ORIGINAL_HEIGHT) /
				static_cast<double>(Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight());

			// Percent of the horizontal weapon offset across the original screen.
			const double weaponOffsetXPercent = static_cast<double>(weaponOffset.x) /
				static_cast<double>(Renderer::ORIGINAL_WIDTH);

			// Native left and right screen edges converted to original space.
			const int newLeft = renderer.nativeToOriginal(Int2(0, 0)).x - 1;
			const int newRight = renderer.nativeToOriginal(
				Int2(renderer.getWindowDimensions().x, 0)).x;

			const int weaponX = newLeft + static_cast<int>(std::round(
				static_cast<double>(newRight - newLeft) * weaponOffsetXPercent));
			const int weaponY = static_cast<int>(std::round(
				static_cast<double>(weaponOffset.y) * weaponScale));
			const int weaponWidth = static_cast<int>(std::round(
				static_cast<double>(weaponTexture.getWidth()) * weaponScale));
			const int weaponHeight = static_cast<int>(std::round(
				static_cast<double>(std::min(weaponTexture.getHeight() + 1,
					Renderer::ORIGINAL_HEIGHT - weaponY)) * weaponScale));

			renderer.drawOriginal(weaponTexture.get(),
				weaponX, weaponY, weaponWidth, weaponHeight);
		}
		else
		{
			// Add 1 to the height because Arena's renderer has an off-by-one bug, and a 1 pixel
			// gap appears unless a small change is added.
			const int weaponHeight = std::min(weaponTexture.getHeight() + 1,
				(Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight()) - weaponOffset.y);
			renderer.drawOriginal(weaponTexture.get(),
				weaponOffset.x, weaponOffset.y, weaponTexture.getWidth(), weaponHeight);
		}
	}

	// Draw the visible portion of the compass slider, and the frame over it.
	if (options.getShowCompass())
	{
		this->drawCompass(player.getGroundDirection(), textureManager, renderer);
	}

	// Draw each pop-up text if its duration is positive.
	// - To do: maybe give delta time to render()? Or store in tick()? I want to avoid 
	//   subtracting the time in tick() because it would always be one frame shorter then.
	auto &triggerText = gameData.getTriggerText();
	auto &actionText = gameData.getActionText();
	if (triggerText.first > 0.0)
	{
		const auto &triggerTextBox = *triggerText.second.get();
		const int centerX = (Renderer::ORIGINAL_WIDTH / 2) -
			(triggerTextBox.getSurface()->w / 2) - 1;
		const int centerY = [modernInterface, &gameInterface, &triggerTextBox]()
		{
			const int interfaceOffset = modernInterface ?
				(gameInterface.getHeight() / 2) : gameInterface.getHeight();
			return Renderer::ORIGINAL_HEIGHT - interfaceOffset -
				triggerTextBox.getSurface()->h - 2;
		}();

		renderer.drawOriginal(triggerTextBox.getTexture(), centerX, centerY);
	}

	if (actionText.first > 0.0)
	{
		const auto &actionTextBox = *actionText.second.get();
		const int textX = (Renderer::ORIGINAL_WIDTH / 2) -
			(actionTextBox.getSurface()->w / 2);
		const int textY = 20;
		renderer.drawOriginal(actionTextBox.getTexture(), textX, textY);
	}

	// To do: draw "effect text" (similar to trigger text).

	// Check if the mouse is over one of the buttons for tooltips in classic mode.
	if (!modernInterface)
	{
		const auto &inputManager = this->getGame().getInputManager();
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

		// Get the hovered tooltip string, or the empty string if none are hovered over.
		const std::string tooltip = [this, &player, &originalPosition]() -> std::string
		{
			if (this->characterSheetButton.contains(originalPosition))
			{
				return "Character Sheet";
			}
			else if (this->drawWeaponButton.contains(originalPosition))
			{
				return "Draw/Sheathe Weapon";
			}
			else if (this->mapButton.contains(originalPosition))
			{
				return "Automap/World Map";
			}
			else if (this->stealButton.contains(originalPosition))
			{
				return "Steal";
			}
			else if (this->statusButton.contains(originalPosition))
			{
				return "Status";
			}
			else if (this->magicButton.contains(originalPosition) &&
				player.getCharacterClass().canCastMagic())
			{
				return "Spells";
			}
			else if (this->logbookButton.contains(originalPosition))
			{
				return "Logbook";
			}
			else if (this->useItemButton.contains(originalPosition))
			{
				return "Use Item";
			}
			else if (this->campButton.contains(originalPosition))
			{
				return "Camp";
			}
			else
			{
				// None are hovered. Return empty string.
				return std::string();
			}
		}();

		if (tooltip.size() > 0)
		{
			this->drawTooltip(tooltip, renderer);
		}
	}
}
