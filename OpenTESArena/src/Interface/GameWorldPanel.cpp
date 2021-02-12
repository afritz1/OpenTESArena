#include <algorithm>
#include <cmath>

#include "SDL.h"

#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "Surface.h"
#include "TextAlignment.h"
#include "TextSubPanel.h"
#include "Texture.h"
#include "WorldMapPanel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CFAFile.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityAnimationInstance.h"
#include "../Entities/EntityAnimationUtils.h"
#include "../Entities/EntityType.h"
#include "../Entities/Player.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/DateUtils.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../Media/AudioManager.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/FontUtils.h"
#include "../Media/MusicUtils.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../World/ArenaInteriorUtils.h"
#include "../World/ArenaLevelUtils.h"
#include "../World/ArenaVoxelUtils.h"
#include "../World/ArenaWildUtils.h"
#include "../World/ChunkUtils.h"
#include "../World/LevelData.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/MapType.h"
#include "../World/SkyUtils.h"
#include "../World/VoxelFacing3D.h"
#include "../World/WeatherUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

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
	constexpr std::array<CursorAlignment, 9> ArrowCursorAlignments =
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

	// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
	// levels where ceilingHeight < 1.0, and same with ceiling blue dots).
	void DEBUG_ColorRaycastPixel(Game &game, Renderer &renderer)
	{
		const int selectionDim = 3;
		const Int2 windowDims = renderer.getWindowDimensions();

		constexpr int xOffset = 16;
		constexpr int yOffset = 16;

		auto &gameData = game.getGameData();
		
		const auto &options = game.getOptions();
		const int chunkDistance = options.getMisc_ChunkDistance();
		const double verticalFOV = options.getGraphics_VerticalFOV();
		const bool pixelPerfect = options.getInput_PixelPerfectSelection();

		const auto &player = gameData.getPlayer();
		const CoordDouble3 &rayStart = player.getPosition();
		const NewDouble3 &cameraDirection = player.getDirection();
		const int viewWidth = windowDims.x;
		const int viewHeight = renderer.getViewHeight();
		const double viewAspectRatio = static_cast<double>(viewWidth) / static_cast<double>(viewHeight);

		const auto &worldData = gameData.getActiveWorld();
		const auto &levelData = worldData.getActiveLevel();
		const auto &entityManager = levelData.getEntityManager();
		const auto &voxelGrid = levelData.getVoxelGrid();
		const double ceilingHeight = levelData.getCeilingHeight();

		const std::string &paletteFilename = ArenaPaletteName::Default;
		auto &textureManager = game.getTextureManager();
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
		if (!paletteID.has_value())
		{
			DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
		}

		const Palette &palette = textureManager.getPaletteHandle(*paletteID);

		for (int y = 0; y < windowDims.y; y += yOffset)
		{
			for (int x = 0; x < windowDims.x; x += xOffset)
			{
				// Position percents across the screen. Add 0.50 to sample at the center of the pixel.
				const double pixelXPercent = (static_cast<double>(x) + 0.50) / static_cast<double>(viewWidth);
				const double pixelYPercent = (static_cast<double>(y) + 0.50) / static_cast<double>(viewHeight);
				const Double3 rayDirection = renderer.screenPointToRay(
					pixelXPercent, pixelYPercent, cameraDirection, verticalFOV, viewAspectRatio);

				// Not registering entities with ray cast hits for efficiency since this debug
				// visualization is for voxels.
				const bool includeEntities = false;

				Physics::Hit hit;
				const bool success = Physics::rayCast(rayStart, rayDirection, chunkDistance, ceilingHeight,
					cameraDirection, pixelPerfect, palette, includeEntities, levelData,
					game.getEntityDefinitionLibrary(), renderer, hit);

				if (success)
				{
					Color color;
					switch (hit.getType())
					{
						case Physics::Hit::Type::Voxel:
						{
							const std::array<Color, 5> colors =
							{
								Color::Red, Color::Green, Color::Blue, Color::Cyan, Color::Yellow
							};

							const CoordInt3 &coord = hit.getVoxelHit().coord;
							const NewInt3 hitVoxel = VoxelUtils::coordToNewVoxel(coord);
							const int colorsIndex = std::min(hitVoxel.y, 4);
							color = colors[colorsIndex];
							break;
						}
						case Physics::Hit::Type::Entity:
						{
							color = Color::Yellow;
							break;
						}
					}

					renderer.drawRect(color, x, y, selectionDim, selectionDim);
				}
			}
		}
	}

	// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
	// levels where ceilingHeight < 1.0, and same with ceiling blue dots).
	void DEBUG_PhysicsRaycast(Game &game, Renderer &renderer)
	{
		// ray cast out from center and display hit info (faster/better than console logging).
		DEBUG_ColorRaycastPixel(game, renderer);

		auto &gameData = game.getGameData();
		const auto &options = game.getOptions();
		const auto &player = gameData.getPlayer();
		const Double3 &cameraDirection = player.getDirection();

		const CoordDouble3 rayStart = player.getPosition();
		const Double3 rayDirection = [&game, &options, &cameraDirection]()
		{
			const auto &renderer = game.getRenderer();
			const Int2 windowDims = renderer.getWindowDimensions();
			const int viewWidth = windowDims.x;
			const int viewHeight = renderer.getViewHeight();
			const double viewAspectRatio = static_cast<double>(viewWidth) /
				static_cast<double>(viewHeight);

			// Position percents across the screen. Add 0.50 to sample at the center
			// of the pixel.
			const double pixelXPercent = (static_cast<double>(viewWidth / 2) + 0.50) /
				static_cast<double>(viewWidth);
			const double pixelYPercent = (static_cast<double>(viewHeight / 2) + 0.50) /
				static_cast<double>(viewHeight);

			return renderer.screenPointToRay(pixelXPercent, pixelYPercent, cameraDirection,
				options.getGraphics_VerticalFOV(), viewAspectRatio);
		}();

		const auto &worldData = gameData.getActiveWorld();
		const auto &levelData = worldData.getActiveLevel();
		const auto &entityManager = levelData.getEntityManager();
		const auto &voxelGrid = levelData.getVoxelGrid();

		const std::string &paletteFilename = ArenaPaletteName::Default;
		auto &textureManager = game.getTextureManager();
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
		if (!paletteID.has_value())
		{
			DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
		}

		const Palette &palette = textureManager.getPaletteHandle(*paletteID);

		const bool includeEntities = true;

		Physics::Hit hit;
		const bool success = Physics::rayCast(rayStart, rayDirection,
			options.getMisc_ChunkDistance(), levelData.getCeilingHeight(), cameraDirection,
			options.getInput_PixelPerfectSelection(), palette, includeEntities, levelData,
			game.getEntityDefinitionLibrary(), renderer, hit);

		std::string text;
		if (success)
		{
			switch (hit.getType())
			{
			case Physics::Hit::Type::Voxel:
			{
				const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
				const CoordInt3 &coord = voxelHit.coord;
				const NewInt3 voxel = VoxelUtils::coordToNewVoxel(voxelHit.coord);
				const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);

				text = "Voxel: (" + voxel.toString() + "), " +
					std::to_string(static_cast<int>(voxelDef.type)) +
					' ' + std::to_string(hit.getT());
				break;
			}
			case Physics::Hit::Type::Entity:
			{
				const Physics::Hit::EntityHit &entityHit = hit.getEntityHit();
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();

				// Try inspecting the entity (can be from any distance). If they have a display name,
				// then show it.
				ConstEntityRef entityRef = entityManager.getEntityRef(entityHit.id, entityHit.type);
				DebugAssert(entityRef.getID() != EntityManager::NO_ID);

				const EntityDefinition &entityDef = entityManager.getEntityDef(
					entityRef.get()->getDefinitionID(), game.getEntityDefinitionLibrary());
				const auto &charClassLibrary = game.getCharacterClassLibrary();

				std::string entityName;
				if (EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
				{
					text = std::move(entityName);
				}
				else
				{
					// Placeholder text for testing.
					text = "Entity " + std::to_string(entityHit.id);
				}

				text.append(' ' + std::to_string(hit.getT()));
				break;
			}
			default:
				text.append("Unknown hit type");
				break;
			}
		}
		else
		{
			text = "No hit";
		}

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Arena,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		const TextBox textBox(0, 0, richText, fontLibrary, renderer);
		const int originalX = ArenaRenderUtils::SCREEN_WIDTH / 2;
		const int originalY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) + 10;
		renderer.drawOriginal(textBox.getTexture(), originalX, originalY);
	}
}

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game)
{
	DebugAssert(game.gameDataIsActive());

	this->playerNameTextBox = [&game]()
	{
		const int x = 17;
		const int y = 154;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			game.getGameData().getPlayer().getFirstName(),
			FontName::Char,
			Color(215, 121, 8),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
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
			DebugLog("Steal.");
		};
		return Button<>(147, 151, 29, 22, function);
	}();

	this->statusButton = [this]()
	{
		auto function = [this](Game &game)
		{
			auto &textureManager = game.getTextureManager();
			const bool modernInterface = game.getOptions().getGraphics_ModernInterface();

			// The center of the pop-up depends on the interface mode.
			const Int2 center = GameWorldPanel::getInterfaceCenter(modernInterface, textureManager);

			const std::string text = [&game]()
			{
				auto &gameData = game.getGameData();
				const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
				const auto &exeData = binaryAssetLibrary.getExeData();
				const LocationDefinition &locationDef = gameData.getLocationDefinition();
				const LocationInstance &locationInst = gameData.getLocationInstance();
				const std::string &locationName = locationInst.getName(locationDef);

				const std::string timeString = [&game, &gameData, &exeData]()
				{
					const Clock &clock = gameData.getClock();
					const int hours = clock.getHours12();
					const int minutes = clock.getMinutes();
					const std::string clockTimeString = std::to_string(hours) + ":" +
						((minutes < 10) ? "0" : "") + std::to_string(minutes);

					const int timeOfDayIndex = [&gameData]()
					{
						// Arena has eight time ranges for each time of day. They aren't
						// uniformly distributed -- midnight and noon are only one minute.
						const std::array<std::pair<Clock, int>, 8> clocksAndIndices =
						{
							std::make_pair(ArenaClockUtils::Midnight, 6),
							std::make_pair(ArenaClockUtils::Night1, 5),
							std::make_pair(ArenaClockUtils::EarlyMorning, 0),
							std::make_pair(ArenaClockUtils::Morning, 1),
							std::make_pair(ArenaClockUtils::Noon, 2),
							std::make_pair(ArenaClockUtils::Afternoon, 3),
							std::make_pair(ArenaClockUtils::Evening, 4),
							std::make_pair(ArenaClockUtils::Night2, 5)
						};

						const Clock &presentClock = gameData.getClock();

						// Reverse iterate, checking which range the active clock is in.
						const auto pairIter = std::find_if(
							clocksAndIndices.rbegin(), clocksAndIndices.rend(),
							[&presentClock](const std::pair<Clock, int> &pair)
						{
							const Clock &clock = pair.first;
							return presentClock.getTotalSeconds() >= clock.getTotalSeconds();
						});

						DebugAssertMsg(pairIter != clocksAndIndices.rend(), "No valid time of day.");
						return pairIter->second;
					}();

					const std::string &timeOfDayString =
						exeData.calendar.timesOfDay.at(timeOfDayIndex);

					return clockTimeString + ' ' + timeOfDayString;
				}();

				// Get the base status text.
				std::string baseText = exeData.status.popUp;

				// Replace carriage returns with newlines.
				baseText = String::replace(baseText, '\r', '\n');

				// Replace first %s with location name.
				size_t index = baseText.find("%s");
				baseText.replace(index, 2, locationName);

				// Replace second %s with time string.
				index = baseText.find("%s", index);
				baseText.replace(index, 2, timeString);

				// Replace third %s with date string, filled in with each value.
				std::string dateString = DateUtils::getDateString(gameData.getDate(), exeData);
				dateString.back() = '\n'; // Replace \r with \n.

				index = baseText.find("%s", index);
				baseText.replace(index, 2, dateString);

				// Replace %d's with current and total weight.
				const int currentWeight = 0;
				index = baseText.find("%d", index);
				baseText.replace(index, 2, std::to_string(currentWeight));

				const int weightCapacity = 0;
				index = baseText.find("%d", index);
				baseText.replace(index, 2, std::to_string(weightCapacity));

				// Append the list of effects at the bottom (healthy/diseased...).
				const std::string effectText = [&exeData]()
				{
					std::string text = exeData.status.effect;

					// Replace carriage returns with newlines.
					text = String::replace(text, '\r', '\n');

					// Replace %s with placeholder.
					const std::string &effectStr = exeData.status.effectsList.front();
					size_t index = text.find("%s");
					text.replace(index, 2, effectStr);

					// Remove newline on end.
					text.pop_back();

					return text;
				}();

				return baseText + effectText;
			}();

			const Color color(251, 239, 77);
			const int lineSpacing = 1;

			const RichTextString richText(
				text,
				FontName::Arena,
				color,
				TextAlignment::Center,
				lineSpacing,
				game.getFontLibrary());

			const Int2 &richTextDimensions = richText.getDimensions();

			auto &renderer = game.getRenderer();
			Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark,
				richTextDimensions.x + 12, richTextDimensions.y + 12, textureManager, renderer);

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
			DebugLog("Magic.");
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
			DebugLog("Use item.");
		};
		return Button<>(147, 175, 29, 22, function);
	}();

	this->campButton = []()
	{
		auto function = []()
		{
			DebugLog("Camp.");
		};
		return Button<>(177, 175, 29, 22, function);
	}();

	this->scrollUpButton = []()
	{
		// Y position is based on height of interface image.
		const int x = 208;
		const int y = (ArenaRenderUtils::SCREEN_HEIGHT - 53) + 3;

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
		const int y = (ArenaRenderUtils::SCREEN_HEIGHT - 53) + 44;

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
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
				const auto &worldData = gameData.getActiveWorld();
				const auto &level = worldData.getActiveLevel();
				const auto &player = gameData.getPlayer();
				const LocationDefinition &locationDef = gameData.getLocationDefinition();
				const LocationInstance &locationInst = gameData.getLocationInstance();

				// Some places (like named/wild dungeons) do not display a name on the automap.
				const std::string automapLocationName = [&gameData, &exeData, &locationDef, &locationInst]()
				{
					const std::string &locationName = locationInst.getName(locationDef);
					const bool isCity = locationDef.getType() == LocationDefinition::Type::City;
					const bool isMainQuestDungeon = locationDef.getType() == LocationDefinition::Type::MainQuestDungeon;
					return (isCity || isMainQuestDungeon) ? locationName : std::string();
				}();

				game.setPanel<AutomapPanel>(game, player.getPosition(), player.getGroundDirection(),
					level.getVoxelGrid(), level.getTransitions(), automapLocationName);
			}
			else
			{
				game.setPanel<WorldMapPanel>(game, nullptr);
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

	if (!weaponAnimation.isRanged())
	{
		// Melee weapon offsets.
		CIFFile cifFile;
		if (!cifFile.init(weaponFilename.c_str()))
		{
			DebugCrash("Could not init .CIF file \"" + weaponFilename + "\".");
		}

		for (int i = 0; i < cifFile.getImageCount(); i++)
		{
			this->weaponOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
		}
	}
	else
	{
		// Ranged weapon offsets.
		CFAFile cfaFile;
		if (!cfaFile.init(weaponFilename.c_str()))
		{
			DebugCrash("Could not init .CFA file \"" + weaponFilename + "\".");
		}

		for (int i = 0; i < cfaFile.getImageCount(); i++)
		{
			this->weaponOffsets.push_back(Int2(cfaFile.getXOffset(), cfaFile.getYOffset()));
		}
	}

	// If in modern mode, lock mouse to center of screen for free-look.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		this->setFreeLookActive(true);
	}
}

GameWorldPanel::~GameWorldPanel()
{
	// If in modern mode, disable free-look.
	auto &game = this->getGame();
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		this->setFreeLookActive(false);
	}
}

Int2 GameWorldPanel::getInterfaceCenter(bool modernInterface, TextureManager &textureManager)
{
	if (modernInterface)
	{
		return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	}
	else
	{
		const TextureBuilderID gameInterfaceTextureBuilderID =
			GameWorldPanel::getGameWorldInterfaceTextureBuilderID(textureManager);
		const TextureBuilder &gameInterfaceTextureBuilder =
			textureManager.getTextureBuilderHandle(gameInterfaceTextureBuilderID);

		return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2,
			(ArenaRenderUtils::SCREEN_HEIGHT - gameInterfaceTextureBuilder.getHeight()) / 2);
	}
}

std::optional<Panel::CursorData> GameWorldPanel::getCurrentCursor() const
{
	// The cursor texture depends on the current mouse position.
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	const Int2 mousePosition = game.getInputManager().getMousePosition();

	if (modernInterface)
	{
		// Do not show cursor in modern mode.
		return std::nullopt;
	}
	else
	{
		const std::string &paletteFilename = ArenaPaletteName::Default;
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
		if (!paletteID.has_value())
		{
			DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
		}

		// See which arrow cursor region the native mouse is in.
		for (int i = 0; i < this->nativeCursorRegions.size(); i++)
		{
			if (this->nativeCursorRegions[i].contains(mousePosition))
			{
				// Get the relevant arrow cursor.
				const std::string &textureFilename = ArenaTextureName::ArrowCursors;
				const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
					textureManager.tryGetTextureBuilderIDs(textureFilename.c_str());
				if (!textureBuilderIDs.has_value())
				{
					DebugCrash("Couldn't get texture builder IDs for \"" + textureFilename + "\".");
				}

				const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(i);
				const CursorAlignment cursorAlignment = ArrowCursorAlignments.at(i);
				return CursorData(textureBuilderID, *paletteID, cursorAlignment);
			}
		}

		// Not in any of the arrow regions.
		return this->getDefaultCursor();
	}
}

TextureBuilderID GameWorldPanel::getGameWorldInterfaceTextureBuilderID(TextureManager &textureManager)
{
	const std::string &textureFilename = ArenaTextureName::GameWorldInterface;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldPanel::getCompassFrameTextureBuilderID() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &textureFilename = ArenaTextureName::CompassFrame;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldPanel::getCompassSliderTextureBuilderID() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &textureFilename = ArenaTextureName::CompassSlider;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldPanel::getPlayerPortraitTextureBuilderID(
	const std::string &portraitsFilename, int portraitID) const
{
	auto &textureManager = this->getGame().getTextureManager();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(portraitsFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + portraitsFilename + "\".");
	}

	return textureBuilderIDs->getID(portraitID);
}

TextureBuilderID GameWorldPanel::getStatusGradientTextureBuilderID(int gradientID) const
{
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &statusGradientsFilename = ArenaTextureName::StatusGradients;
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(statusGradientsFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + statusGradientsFilename + "\".");
	}

	return textureBuilderIDs->getID(gradientID);
}

TextureBuilderID GameWorldPanel::getNoSpellTextureBuilderID() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &textureFilename = ArenaTextureName::NoSpell;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	return *textureBuilderID;
}

TextureBuilderID GameWorldPanel::getWeaponTextureBuilderID(const std::string &weaponFilename, int index) const
{
	auto &textureManager = this->getGame().getTextureManager();
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(weaponFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + weaponFilename + "\".");
	}

	return textureBuilderIDs->getID(index);
}

void GameWorldPanel::updateCursorRegions(int width, int height)
{
	// Scale ratios.
	const double xScale = static_cast<double>(width) / static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
	const double yScale = static_cast<double>(height) / static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);

	// Lambda for making a cursor region that scales to the current resolution.
	auto scaleRect = [xScale, yScale](const Rect &rect)
	{
		const int x = static_cast<int>(std::ceil(static_cast<double>(rect.getLeft()) * xScale));
		const int y = static_cast<int>(std::ceil(static_cast<double>(rect.getTop()) * yScale));
		const int width = static_cast<int>(std::ceil(static_cast<double>(rect.getWidth()) * xScale));
		const int height = static_cast<int>(std::ceil(static_cast<double>(rect.getHeight()) * yScale));
		return Rect(x, y, width, height);
	};

	// @todo: maybe replace this with an index array into the global regions and a std::generate?

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

void GameWorldPanel::setFreeLookActive(bool active)
{
	auto &game = this->getGame();

	// Set relative mouse mode. When enabled, this freezes the hardware cursor in place but
	// relative motion events are still recorded.
	auto &inputManager = game.getInputManager();
	inputManager.setRelativeMouseMode(active);

	// Warp mouse to center of screen.
	auto &renderer = game.getRenderer();
	const Int2 windowDims = renderer.getWindowDimensions();
	renderer.warpMouse(windowDims.x / 2, windowDims.y / 2);
}

void GameWorldPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	auto &options = game.getOptions();
	auto &player = game.getGameData().getPlayer();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool f4Pressed = inputManager.keyPressed(e, SDLK_F4);

	if (escapePressed)
	{
		this->pauseButton.click(game);
	}
	else if (f4Pressed)
	{
		// Increment or wrap profiler level.
		const int oldProfilerLevel = options.getMisc_ProfilerLevel();
		const int newProfilerLevel = (oldProfilerLevel < Options::MAX_PROFILER_LEVEL) ?
			(oldProfilerLevel + 1) : Options::MIN_PROFILER_LEVEL;
		options.setMisc_ProfilerLevel(newProfilerLevel);
	}

	// Listen for hotkeys.
	const bool drawWeaponHotkeyPressed = inputManager.keyPressed(e, SDLK_f);
	const bool automapHotkeyPressed = inputManager.keyPressed(e, SDLK_n);
	const bool logbookHotkeyPressed = inputManager.keyPressed(e, SDLK_l);
	const bool sheetHotkeyPressed = inputManager.keyPressed(e, SDLK_TAB) ||
		inputManager.keyPressed(e, SDLK_F1);
	const bool statusHotkeyPressed = inputManager.keyPressed(e, SDLK_v);
	const bool worldMapHotkeyPressed = inputManager.keyPressed(e, SDLK_m);
	const bool toggleCompassHotkeyPressed = inputManager.keyPressed(e, SDLK_F8);

	if (drawWeaponHotkeyPressed)
	{
		this->drawWeaponButton.click(player);
	}
	else if (automapHotkeyPressed)
	{
		const bool goToAutomap = true;
		this->mapButton.click(game, goToAutomap);
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
		const bool goToAutomap = false;
		this->mapButton.click(game, goToAutomap);
	}
	else if (toggleCompassHotkeyPressed)
	{
		// Toggle compass display.
		options.setMisc_ShowCompass(!options.getMisc_ShowCompass());
	}

	// Player's XY coordinate hotkey.
	const bool f2Pressed = inputManager.keyPressed(e, SDLK_F2);

	if (f2Pressed)
	{
		// Refresh player coordinates display (probably intended for debugging in the
		// original game). These coordinates are in Arena's coordinate system.
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const auto &worldData = game.getGameData().getActiveWorld();

		const std::string text = [&worldData, &player, &exeData]()
		{
			const auto &level = worldData.getActiveLevel();
			const auto &voxelGrid = level.getVoxelGrid();

			const OriginalInt2 displayedCoords = [&worldData, &player, &voxelGrid]()
			{
				const NewDouble3 absolutePlayerPosition = VoxelUtils::coordToNewPoint(player.getPosition());
				const NewInt3 absolutePlayerVoxel = VoxelUtils::pointToVoxel(absolutePlayerPosition);
				const NewInt2 playerVoxelXZ(absolutePlayerVoxel.x, absolutePlayerVoxel.z);
				const OriginalInt2 originalVoxel = VoxelUtils::newVoxelToOriginalVoxel(playerVoxelXZ);

				// The displayed coordinates in the wilderness behave differently in the original
				// game due to how the 128x128 grid shifts to keep the player roughly centered.
				if (worldData.getMapType() != MapType::Wilderness)
				{
					return originalVoxel;
				}
				else
				{
					const int halfWidth = RMDFile::WIDTH / 2;
					const int halfDepth = RMDFile::DEPTH / 2;
					return OriginalInt2(
						halfWidth + ((originalVoxel.x + halfWidth) % RMDFile::WIDTH),
						halfDepth + ((originalVoxel.y + halfDepth) % RMDFile::DEPTH));
				}
			}();

			std::string str = exeData.ui.currentWorldPosition;

			// Replace first %d with X, second %d with Y.
			size_t index = str.find("%d");
			str.replace(index, 2, std::to_string(displayedCoords.x));

			index = str.find("%d", index);
			str.replace(index, 2, std::to_string(displayedCoords.y));

			return str;
		}();

		auto &gameData = game.getGameData();
		gameData.setActionText(text, game.getFontLibrary(), game.getRenderer());
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// @temp: hold this key down to make clicks cause voxels to fade.
	const bool debugFadeVoxel = inputManager.keyIsDown(SDL_SCANCODE_G);

	const auto &renderer = game.getRenderer();

	// Handle input events based on which player interface mode is active.
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Get mouse position relative to letterbox coordinates.
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

		const Rect &centerCursorRegion = this->nativeCursorRegions.at(4);

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
				const bool goToAutomap = true;
				this->mapButton.click(game, goToAutomap);
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
			else
			{
				// Check for left clicks in the game world.
				if (centerCursorRegion.contains(mousePosition))
				{
					const bool primaryClick = true;
					this->handleClickInWorld(mousePosition, primaryClick, debugFadeVoxel);
				}
			}
		}
		else if (rightClick)
		{
			if (this->mapButton.contains(originalPosition))
			{
				this->mapButton.click(game, false);
			}
			else
			{
				// Check for right clicks in the game world.
				if (centerCursorRegion.contains(mousePosition))
				{
					const bool primaryClick = false;
					this->handleClickInWorld(mousePosition, primaryClick, false);
				}
			}
		}
	}
	else
	{
		// Check modern mode input events.
		const bool ePressed = inputManager.keyPressed(e, SDLK_e);

		// Any clicks will be at the center of the window.
		const Int2 windowDims = renderer.getWindowDimensions();
		const Int2 nativeCenter = windowDims / 2;

		if (ePressed)
		{
			// Activate (left click in classic mode).
			const bool primaryClick = true;
			this->handleClickInWorld(nativeCenter, primaryClick, debugFadeVoxel);
		}
		else if (leftClick)
		{
			// Read (right click in classic mode).
			const bool primaryClick = false;
			this->handleClickInWorld(nativeCenter, primaryClick, false);
		}
	}
}

void GameWorldPanel::onPauseChanged(bool paused)
{
	auto &game = this->getGame();

	// If in modern mode, set free-look to the given value.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		this->setFreeLookActive(!paused);
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
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (!modernInterface)
	{
		// Classic interface mode.
		auto &player = game.getGameData().getPlayer();
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);
		const bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		const bool right = inputManager.keyIsDown(SDL_SCANCODE_D);

		// Don't turn if LCtrl is held.
		const bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// Mouse turning takes priority over key turning.
		if (leftClick)
		{
			const Int2 mousePosition = inputManager.getMousePosition();

			// Strength of turning is determined by proximity of the mouse cursor to
			// the left or right screen edge.
			const double dx = [this, &mousePosition]()
			{
				// Measure the magnitude of rotation. -1.0 is left, 1.0 is right.
				const double percent = [this, &mousePosition]()
				{
					const int mouseX = mousePosition.x;

					// Native cursor regions for turning (scaled to the current window).
					const Rect &topLeft = this->nativeCursorRegions.at(0);
					const Rect &topRight = this->nativeCursorRegions.at(2);
					const Rect &middleLeft = this->nativeCursorRegions.at(3);
					const Rect &middleRight = this->nativeCursorRegions.at(5);

					if (topLeft.contains(mousePosition))
					{
						return -1.0 + (static_cast<double>(mouseX) / topLeft.getWidth());
					}
					else if (topRight.contains(mousePosition))
					{
						return static_cast<double>(mouseX - topRight.getLeft()) /
							topRight.getWidth();
					}
					else if (middleLeft.contains(mousePosition))
					{
						return -1.0 + (static_cast<double>(mouseX) / middleLeft.getWidth());
					}
					else if (middleRight.contains(mousePosition))
					{
						return static_cast<double>(mouseX - middleRight.getLeft()) /
							middleRight.getWidth();
					}
					else
					{
						return 0.0;
					}
				}();

				// No NaNs or infinities allowed.
				return std::isfinite(percent) ? percent : 0.0;
			}();

			// Yaw the camera left or right. No vertical movement in classic camera mode.
			// Multiply turning speed by delta time so it behaves correctly with different
			// frame rates.
			player.rotate(dx * dt, 0.0, options.getInput_HorizontalSensitivity(),
				options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
		}
		else if (!lCtrl)
		{
			// If left control is not held, then turning is permitted.
			if (left)
			{
				// Turn left at a fixed angular velocity.
				player.rotate(-dt, 0.0, options.getInput_HorizontalSensitivity(),
					options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
			}
			else if (right)
			{
				// Turn right at a fixed angular velocity.
				player.rotate(dt, 0.0, options.getInput_HorizontalSensitivity(),
					options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
			}
		}
	}
	else
	{
		// Modern interface. Make the camera look around if the player's weapon is not in use.
		const int dx = mouseDelta.x;
		const int dy = mouseDelta.y;
		const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

		auto &player = game.getGameData().getPlayer();
		const auto &weaponAnim = player.getWeaponAnimation();
		const bool turning = ((dx != 0) || (dy != 0)) && (weaponAnim.isSheathed() || !rightClick);

		if (turning)
		{
			const Int2 dimensions = game.getRenderer().getWindowDimensions();

			// Get the smaller of the two dimensions, so the look sensitivity is relative
			// to a square instead of a rectangle. This keeps the camera look independent
			// of the aspect ratio.
			const int minDimension = std::min(dimensions.x, dimensions.y);
			const double dxx = static_cast<double>(dx) / static_cast<double>(minDimension);
			const double dyy = static_cast<double>(dy) / static_cast<double>(minDimension);

			// Pitch and/or yaw the camera.
			player.rotate(dxx, -dyy, options.getInput_HorizontalSensitivity(),
				options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
		}
	}
}

void GameWorldPanel::handlePlayerMovement(double dt)
{
	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicMovement()
	// 2) handleModernMovement()

	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();

	// Arbitrary movement speeds.
	const double walkSpeed = 15.0;
	const double runSpeed = 30.0;

	const auto &worldData = game.getGameData().getActiveWorld();

	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
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

		auto &player = game.getGameData().getPlayer();

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

			// Only attempt to accelerate if a direction was chosen.
			if (accelDirection.lengthSquared() > 0.0)
			{
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
				else if (std::isfinite(accelDirection.length()) && std::isfinite(accelMagnitude))
				{
					player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
				}
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

		auto &player = game.getGameData().getPlayer();

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
	// @todo: run this method at fixed time-steps instead of every frame, because if,
	// for example, the game is running at 200 fps, then the player has to move their
	// cursor much faster for it to count as a swing. The GameWorldPanel would probably
	// need to save its own "swing" mouse delta independently of the input manager, or
	// maybe the game loop could call a "Panel::fixedTick()" method.

	// Only handle attacking if the player's weapon is currently idle.
	auto &weaponAnimation = this->getGame().getGameData().getPlayer().getWeaponAnimation();
	if (weaponAnimation.isIdle())
	{
		const auto &inputManager = this->getGame().getInputManager();
		auto &audioManager = this->getGame().getAudioManager();

		if (!weaponAnimation.isRanged())
		{
			// Handle melee attack.
			const Int2 dimensions = this->getGame().getRenderer().getWindowDimensions();

			// Get the smaller of the two dimensions, so the percentage change in mouse position
			// is relative to a square instead of a rectangle.
			const int minDimension = std::min(dimensions.x, dimensions.y);

			// Percentages that the mouse moved across the screen.
			const double dxx = static_cast<double>(mouseDelta.x) /
				static_cast<double>(minDimension);
			const double dyy = static_cast<double>(mouseDelta.y) /
				static_cast<double>(minDimension);

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
				// for convenience. This is actually a little weird now because +X is south
				// and +Y is west).
				CardinalDirectionName cardinalDirection = CardinalDirection::getDirectionName(
					Double2(-mouseDirection.y, -mouseDirection.x));

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
				audioManager.playSound(ArenaSoundName::Swish);
			}
		}
		else
		{
			// Handle ranged attack.
			const bool isAttack = [this, &inputManager]()
			{
				auto &game = this->getGame();
				const auto &options = game.getOptions();
				const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

				if (!options.getGraphics_ModernInterface())
				{
					// The cursor must be above the game world interface in order to fire. In
					// the original game, the cursor has to be in the center "X" region, but
					// that seems pretty inconvenient, given that the border between cursor
					// regions is hard to see at a glance, and that might be the difference
					// between shooting an arrow and not shooting an arrow, so I'm relaxing
					// the requirements here.
					auto &textureManager = game.getTextureManager();
					auto &renderer = game.getRenderer();
					const TextureBuilderID gameWorldInterfaceTextureID =
						GameWorldPanel::getGameWorldInterfaceTextureBuilderID(textureManager);
					const TextureBuilder &gameWorldInterfaceTextureBuilder =
						textureManager.getTextureBuilderHandle(gameWorldInterfaceTextureID);
					const int originalCursorY = renderer.nativeToOriginal(inputManager.getMousePosition()).y;
					return rightClick && (originalCursorY <
						(ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilder.getHeight()));
				}
				else
				{
					// Right clicking anywhere in modern mode fires an arrow.
					return rightClick;
				}
			}();

			if (isAttack)
			{
				// Set firing state for animation.
				weaponAnimation.setState(WeaponAnimation::State::Firing);

				// Play the firing sound.
				audioManager.playSound(ArenaSoundName::ArrowFire);
			}
		}
	}
}

void GameWorldPanel::handleClickInWorld(const Int2 &nativePoint, bool primaryClick,
	bool debugFadeVoxel)
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	const auto &options = game.getOptions();
	auto &player = gameData.getPlayer();
	const Double3 &cameraDirection = player.getDirection();
	auto &worldData = gameData.getActiveWorld();
	auto &level = worldData.getActiveLevel();
	auto &voxelGrid = level.getVoxelGrid();
	const auto &entityManager = level.getEntityManager();
	const double ceilingHeight = level.getCeilingHeight();

	const CoordDouble3 rayStart = player.getPosition();
	const NewDouble3 rayDirection = [&nativePoint, &game, &options, &cameraDirection]()
	{
		const auto &renderer = game.getRenderer();
		const Int2 windowDims = renderer.getWindowDimensions();
		const int viewWidth = windowDims.x;
		const int viewHeight = renderer.getViewHeight();
		const double viewAspectRatio = static_cast<double>(viewWidth) /
			static_cast<double>(viewHeight);

		// Mouse position percents across the screen. Add 0.50 to sample at the center
		// of the pixel.
		const double mouseXPercent = (static_cast<double>(nativePoint.x) + 0.50) /
			static_cast<double>(viewWidth);
		const double mouseYPercent = (static_cast<double>(nativePoint.y) + 0.50) /
			static_cast<double>(viewHeight);

		return renderer.screenPointToRay(mouseXPercent, mouseYPercent, cameraDirection,
			options.getGraphics_VerticalFOV(), viewAspectRatio);
	}();

	const int chunkDistance = options.getMisc_ChunkDistance();

	// Pixel-perfect selection determines whether an entity's texture is used in the
	// selection calculation.
	const bool pixelPerfectSelection = options.getInput_PixelPerfectSelection();

	const std::string &paletteFilename = ArenaPaletteName::Default;
	auto &textureManager = game.getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const bool includeEntities = true;

	Physics::Hit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, chunkDistance, ceilingHeight,
		cameraDirection, pixelPerfectSelection, palette, includeEntities, level,
		game.getEntityDefinitionLibrary(), game.getRenderer(), hit);

	// See if the ray hit anything.
	if (success)
	{
		if (hit.getType() == Physics::Hit::Type::Voxel)
		{
			const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
			const CoordInt3 &coord = voxelHit.coord;
			const NewInt3 voxel = VoxelUtils::coordToNewVoxel(coord);
			const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);
			const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);

			// Primary click handles selection in the game world. Secondary click handles
			// reading names of things.
			if (primaryClick)
			{
				// Arbitrary max distance for selection.
				// @todo: move to some ArenaPlayerUtils maybe
				constexpr double maxSelectionDist = 1.50;

				if (hit.getT() <= maxSelectionDist)
				{
					if (voxelDef.type == ArenaTypes::VoxelType::Wall ||
						voxelDef.type == ArenaTypes::VoxelType::Floor ||
						voxelDef.type == ArenaTypes::VoxelType::Raised ||
						voxelDef.type == ArenaTypes::VoxelType::Diagonal ||
						voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
					{
						if (!debugFadeVoxel)
						{
							if (voxelDef.type == ArenaTypes::VoxelType::Wall)
							{
								const LevelData::Transitions &transitions = level.getTransitions();
								const auto transitionIter = transitions.find(NewInt2(voxel.x, voxel.z));
								const bool isMenu = (transitionIter != transitions.end()) &&
									(transitionIter->second.getType() == LevelData::Transition::Type::Menu);

								if (isMenu)
								{
									const LevelData::Transition::Menu &transitionMenu = transitionIter->second.getMenu();
									this->handleWorldTransition(hit, transitionMenu.id);
								}
							}
						}
						else
						{
							// @temp: add to fading voxels if it doesn't already exist.
							const VoxelInstance *existingFadingVoxelInst =
								level.tryGetVoxelInstance(voxel, VoxelInstance::Type::Fading);
							const bool isFading = existingFadingVoxelInst != nullptr;
							if (!isFading)
							{
								VoxelInstance newFadingVoxelInst = VoxelInstance::makeFading(
									voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::FADING_VOXEL_SECONDS);
								level.addVoxelInstance(std::move(newFadingVoxelInst));
							}
						}
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
					{
						const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

						if (edgeData.collider)
						{
							// The only collidable edges in cities should be palace voxels. Not sure
							// how the original game handles the menu ID since it's a type 0xA voxel.
							const int menuID = 11;
							this->handleWorldTransition(hit, menuID);
						}
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Door)
					{
						const VoxelDefinition::DoorData &doorData = voxelDef.door;

						// If the door is closed, then open it.
						const VoxelInstance *existingOpenDoorInst =
							level.tryGetVoxelInstance(voxel, VoxelInstance::Type::OpenDoor);
						const bool isClosed = existingOpenDoorInst == nullptr;

						if (isClosed)
						{
							// Add the door to the open doors list.
							VoxelInstance newOpenDoorInst = VoxelInstance::makeDoor(
								voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::DOOR_ANIM_SPEED);
							level.addVoxelInstance(std::move(newOpenDoorInst));

							// Play the door's opening sound at the center of the voxel.
							const auto &doorSoundLibrary = game.getDoorSoundLibrary();
							const std::optional<int> doorSoundDefIndex =
								doorSoundLibrary.tryGetDefIndex(doorData.type, DoorSoundDefinition::Type::Open);

							if (doorSoundDefIndex.has_value())
							{
								const DoorSoundDefinition &doorSoundDef = doorSoundLibrary.getDef(*doorSoundDefIndex);
								const DoorSoundDefinition::OpenDef &openDoorSoundDef = doorSoundDef.getOpen();
								const auto &inf = level.getInfFile();
								auto &audioManager = game.getAudioManager();
								const std::string &soundFilename = inf.getSound(openDoorSoundDef.soundIndex);
								const double ceilingHeight = level.getCeilingHeight();
								const Double3 soundPosition(
									static_cast<SNDouble>(voxel.x) + 0.50,
									(static_cast<double>(voxel.y) * ceilingHeight) + (ceilingHeight * 0.50),
									static_cast<WEDouble>(voxel.z) + 0.50);

								audioManager.playSound(soundFilename, soundPosition);
							}
						}
					}
				}
			}
			else
			{
				// Handle secondary click (i.e., right click).
				if (voxelDef.type == ArenaTypes::VoxelType::Wall)
				{
					const LevelData::Transitions &transitions = level.getTransitions();
					const auto transitionIter = transitions.find(NewInt2(voxel.x, voxel.z));
					const bool isMenu = (transitionIter != transitions.end()) &&
						(transitionIter->second.getType() == LevelData::Transition::Type::Menu);
					const bool isInterior = worldData.getMapType() == MapType::Interior;

					// Print interior display name if *MENU block is clicked in an exterior.
					if (isMenu && !isInterior)
					{
						const LevelData::Transition::Menu &transitionMenu = transitionIter->second.getMenu();
						const MapType mapType = worldData.getMapType();
						const auto menuType = ArenaVoxelUtils::getMenuType(transitionMenu.id, mapType);

						if (ArenaVoxelUtils::menuHasDisplayName(menuType))
						{
							// Get interior name from the clicked voxel.
							const std::string menuName = [&game, &level, &voxel, mapType, menuType]()
							{
								const NewInt2 voxelXZ(voxel.x, voxel.z);

								if (mapType == MapType::City)
								{
									// City interior name.
									const auto &menuNames = level.getMenuNames();
									const auto iter = std::find_if(menuNames.begin(), menuNames.end(),
										[&voxelXZ](const std::pair<NewInt2, std::string> &pair)
									{
										return pair.first == voxelXZ;
									});

									const bool foundName = iter != menuNames.end();

									if (foundName)
									{
										return iter->second;
									}
									else
									{
										// If no menu name was generated, then see if it's a mage's guild.
										if (menuType == ArenaTypes::MenuType::MagesGuild)
										{
											const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
											const auto &exeData = binaryAssetLibrary.getExeData();
											return exeData.cityGen.magesGuildMenuName;
										}
										else
										{
											// This should only happen if the player created a new *MENU voxel,
											// which shouldn't occur in regular play.
											DebugLogWarning("No *MENU name at (" + std::to_string(voxelXZ.x) +
												", " + std::to_string(voxelXZ.y) + ").");
											return std::string();
										}
									}
								}
								else if (mapType == MapType::Wilderness)
								{
									// Wilderness interior name.

									// Probably don't need this here with the current wild name generation.
									/*const auto &voxelGrid = exterior.getVoxelGrid();
									const Int2 originalVoxel = VoxelGrid::getTransformedCoordinate(
										voxelXZ, voxelGrid.getWidth(), voxelGrid.getDepth());
									const Int2 relativeOrigin = ExteriorLevelData::getRelativeWildOrigin(originalVoxel);
									const Int2 relativeVoxel = originalVoxel - relativeOrigin;
									const Int2 chunkCoords(
										originalVoxel.x / RMDFile::WIDTH,
										originalVoxel.y / RMDFile::DEPTH);*/

									const auto &menuNames = level.getMenuNames();
									const auto iter = std::find_if(menuNames.begin(), menuNames.end(),
										[&voxelXZ](const std::pair<NewInt2, std::string> &pair)
									{
										return pair.first == voxelXZ;
									});

									const bool foundName = iter != menuNames.end();

									if (foundName)
									{
										return iter->second;
									}
									else
									{
										// This should only happen if the player created a new *MENU voxel,
										// which shouldn't occur in regular play.
										DebugLogWarning("No *MENU name at (" + std::to_string(voxelXZ.x) +
											", " + std::to_string(voxelXZ.y) + ").");
										return std::string();
									}
								}
								else
								{
									DebugUnhandledReturnMsg(std::string,
										std::to_string(static_cast<int>(mapType)));
								}
							}();

							auto &gameData = game.getGameData();
							gameData.setActionText(menuName, game.getFontLibrary(), game.getRenderer());
						}
					}
				}
			}
		}
		else if (hit.getType() == Physics::Hit::Type::Entity)
		{
			const Physics::Hit::EntityHit &entityHit = hit.getEntityHit();
			const auto &exeData = game.getBinaryAssetLibrary().getExeData();

			if (primaryClick)
			{
				// @todo: max selection distance matters when talking to NPCs and selecting corpses.
				// - need to research a bit since I think it switches between select and inspect
				//   depending on distance and entity state.
				// - Also need the "too far away..." text?
				/*const double maxSelectionDist = 1.50;
				if (hit.t <= maxSelectionDist)
				{

				}*/

				// Try inspecting the entity (can be from any distance). If they have a display name,
				// then show it.
				ConstEntityRef entityRef = entityManager.getEntityRef(entityHit.id, entityHit.type);
				DebugAssert(entityRef.getID() != EntityManager::NO_ID);

				const EntityDefinition &entityDef = entityManager.getEntityDef(
					entityRef.get()->getDefinitionID(), game.getEntityDefinitionLibrary());
				const auto &charClassLibrary = game.getCharacterClassLibrary();

				std::string entityName;
				std::string text;
				if (EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
				{
					text = exeData.ui.inspectedEntityName;

					// Replace format specifier with entity name.
					text = String::replace(text, "%s", entityName);
				}
				else
				{
					// Placeholder text for testing.
					text = "Entity " + std::to_string(entityHit.id) + " (" +
						EntityUtils::defTypeToString(entityDef) + ")";
				}

				auto &gameData = game.getGameData();
				gameData.setActionText(text, game.getFontLibrary(), game.getRenderer());
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.getType())));
		}
	}
}

void GameWorldPanel::handleNightLightChange(bool active)
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &gameData = game.getGameData();
	auto &worldData = gameData.getActiveWorld();
	auto &levelData = worldData.getActiveLevel();
	auto &entityManager = levelData.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();

	// Turn streetlights on or off.
	Buffer<Entity*> entityBuffer(entityManager.getCount(EntityType::Static));
	const int entityCount = entityManager.getEntities(
		EntityType::Static, entityBuffer.get(), entityBuffer.getCount());

	for (int i = 0; i < entityCount; i++)
	{
		Entity *entity = entityBuffer.get(i);
		const EntityDefID defID = entity->getDefinitionID();
		const EntityDefinition &entityDef = entityManager.getEntityDef(defID, entityDefLibrary);

		if ((entityDef.getType() == EntityDefinition::Type::Doodad) &&
			(entityDef.getDoodad().streetlight))
		{
			const std::string &newStateName = active ?
				EntityAnimationUtils::STATE_ACTIVATED : EntityAnimationUtils::STATE_IDLE;

			const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
			int newStateIndex;
			if (!animDef.tryGetStateIndex(newStateName.c_str(), &newStateIndex))
			{
				DebugLogWarning("Missing entity animation state \"" + newStateName + "\".");
				continue;
			}

			EntityAnimationInstance &animInst = entity->getAnimInstance();
			animInst.setStateIndex(newStateIndex);
		}
	}

	TextureManager &textureManager = game.getTextureManager();
	const std::string paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	renderer.setNightLightsActive(active, palette);
}

void GameWorldPanel::handleTriggers(const NewInt2 &voxel)
{
	auto &game = this->getGame();
	auto &worldData = game.getGameData().getActiveWorld();

	// Only interior levels have triggers.
	if (worldData.getMapType() == MapType::Interior)
	{
		auto &level = worldData.getActiveLevel();

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

				auto &gameData = game.getGameData();
				gameData.setTriggerText(text, game.getFontLibrary(), game.getRenderer());

				// Set the text trigger as activated (regardless of whether or not it's
				// single-shot, just for consistency).
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
}

void GameWorldPanel::handleDoors(double dt, const Double2 &playerPos)
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	auto &worldData = gameData.getActiveWorld();
	auto &activeLevel = worldData.getActiveLevel();

	// Lambda for playing a sound by .INF sound index if the close sound types match.
	auto playCloseSoundIfType = [&game, &activeLevel](const DoorSoundDefinition::CloseDef &closeSoundDef, 
		DoorSoundDefinition::CloseType closeType, const NewInt3 &doorVoxel)
	{
		if (closeSoundDef.closeType == closeType)
		{
			const auto &inf = activeLevel.getInfFile();
			const std::string &soundFilename = inf.getSound(closeSoundDef.soundIndex);
			auto &audioManager = game.getAudioManager();

			// Put at the center of the door voxel.
			const double ceilingHeight = activeLevel.getCeilingHeight();
			const Double3 soundPosition(
				static_cast<SNDouble>(doorVoxel.x) + 0.50,
				(static_cast<double>(doorVoxel.y) * ceilingHeight) + (ceilingHeight * 0.50),
				static_cast<WEDouble>(doorVoxel.z) + 0.50);

			audioManager.playSound(soundFilename, soundPosition);
		}
	};

	const ChunkInt2 playerChunk = VoxelUtils::newVoxelToChunk(
		NewInt2(static_cast<SNInt>(playerPos.x), static_cast<WEInt>(playerPos.y)));
	const int chunkDistance = game.getOptions().getMisc_ChunkDistance();
	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(playerChunk, chunkDistance, &minChunk, &maxChunk);

	// Iterate over chunks near the player.
	for (SNInt chunkX = minChunk.x; chunkX <= maxChunk.x; chunkX++)
	{
		for (WEInt chunkZ = minChunk.y; chunkZ <= maxChunk.y; chunkZ++)
		{
			const ChunkInt2 chunk(chunkX, chunkZ);
			LevelData::VoxelInstanceGroup *voxelInstGroup = activeLevel.tryGetVoxelInstances(chunk);
			if (voxelInstGroup != nullptr)
			{
				for (auto &pair : *voxelInstGroup)
				{
					std::vector<VoxelInstance> &voxelInsts = pair.second;

					// Update each open door and remove ones that become closed.
					for (int i = static_cast<int>(voxelInsts.size()) - 1; i >= 0; i--)
					{
						VoxelInstance &voxelInst = voxelInsts[i];
						if (voxelInst.getType() == VoxelInstance::Type::OpenDoor)
						{
							voxelInst.update(dt);

							// Get the door's voxel definition and its sound definition for determining how it
							// sounds when closing.
							const auto &voxelGrid = activeLevel.getVoxelGrid();
							const NewInt3 voxel(voxelInst.getX(), voxelInst.getY(), voxelInst.getZ());
							const uint16_t voxelID = voxelGrid.getVoxel(voxel.x, voxel.y, voxel.z);
							const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
							const VoxelDefinition::DoorData &doorData = voxelDef.door;

							const auto &doorSoundLibrary = game.getDoorSoundLibrary();
							const std::optional<int> doorSoundDefIndex =
								doorSoundLibrary.tryGetDefIndex(doorData.type, DoorSoundDefinition::Type::Close);
							const DoorSoundDefinition *doorSoundDef = doorSoundDefIndex.has_value() ?
								&doorSoundLibrary.getDef(*doorSoundDefIndex) : nullptr;

							VoxelInstance::DoorState &doorState = voxelInst.getDoorState();
							const VoxelInstance::DoorState::StateType doorStateType = doorState.getStateType();
							if (doorStateType == VoxelInstance::DoorState::StateType::Closed)
							{
								if (doorSoundDef != nullptr)
								{
									// Only some doors play a sound when they become closed.
									playCloseSoundIfType(doorSoundDef->getClose(), DoorSoundDefinition::CloseType::OnClosed, voxel);
								}

								// Erase closed door.
								voxelInsts.erase(voxelInsts.begin() + i);
							}
							else if (doorStateType != VoxelInstance::DoorState::StateType::Closing)
							{
								// Auto-close doors that the player is far enough away from.
								const bool farEnough = [&playerPos, &voxel]()
								{
									constexpr double maxDistance = ArenaLevelUtils::DOOR_CLOSE_DISTANCE;
									constexpr double maxDistanceSqr = maxDistance * maxDistance;
									const Double2 diff = playerPos - VoxelUtils::getVoxelCenter(NewInt2(voxel.x, voxel.z));
									const double distSqr = (diff.x * diff.x) + (diff.y * diff.y);
									return distSqr > maxDistanceSqr;
								}();

								if (farEnough)
								{
									doorState.setStateType(VoxelInstance::DoorState::StateType::Closing);

									if (doorSoundDef != nullptr)
									{
										// Only some doors play a sound when they start closing.
										playCloseSoundIfType(doorSoundDef->getClose(), DoorSoundDefinition::CloseType::OnClosing, voxel);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void GameWorldPanel::handleWorldTransition(const Physics::Hit &hit, int menuID)
{
	// @todo: maybe will need to change this to account for wilderness dens?
	DebugAssert(hit.getType() == Physics::Hit::Type::Voxel);
	const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();

	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	auto &worldData = gameData.getActiveWorld();
	auto &activeLevel = worldData.getActiveLevel();
	const MapType activeMapType = worldData.getMapType();

	const LocationDefinition &locationDef = gameData.getLocationDefinition();
	DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

	// Decide based on the active world type.
	if (activeMapType == MapType::Interior)
	{
		// @temp: temporary condition while test interiors are allowed on the main menu.
		if (!gameData.isActiveWorldNested())
		{
			DebugLogWarning("Test interiors have no exterior.");
			return;
		}

		// Leave the interior and go to the saved exterior.
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		gameData.leaveInterior(game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
			binaryAssetLibrary, game.getRandom(), textureManager, renderer);

		// Change to exterior music.
		const auto &clock = gameData.getClock();
		const WeatherType filteredWeatherType = WeatherUtils::getFilteredWeatherType(
			gameData.getWeatherType(), cityDef.climateType);

		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = [&game, &gameData, filteredWeatherType, &musicLibrary]()
		{
			if (!gameData.nightMusicIsActive())
			{
				return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [filteredWeatherType](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.type == filteredWeatherType;
				});
			}
			else
			{
				return musicLibrary.getRandomMusicDefinition(
					MusicDefinition::Type::Night, game.getRandom());
			}
		}();

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		// Only play jingle if the exterior is inside the city.
		const MusicDefinition *jingleMusicDef = nullptr;
		if (gameData.getActiveWorld().getMapType() == MapType::City)
		{
			jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
				game.getRandom(), [&cityDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
				const auto &jingleMusicDef = def.getJingleMusicDefinition();
				return (jingleMusicDef.cityType == cityDef.type) &&
					(jingleMusicDef.climateType == cityDef.climateType);
			});

			if (jingleMusicDef == nullptr)
			{
				DebugLogWarning("Missing jingle music.");
			}
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef, jingleMusicDef);
	}
	else
	{
		// Either city or wilderness. If the menu ID is for an interior, enter it. If it's
		// the city gates, toggle between city and wilderness. If it's "none", then do nothing.
		const ArenaTypes::MenuType menuType = ArenaVoxelUtils::getMenuType(menuID, activeMapType);
		const bool isTransitionVoxel = menuType != ArenaTypes::MenuType::None;

		// Make sure the voxel will actually lead somewhere first.
		if (isTransitionVoxel)
		{
			const bool isTransitionToInterior = ArenaVoxelUtils::menuLeadsToInterior(menuType);
			const CoordInt3 &coord = voxelHit.coord;
			const NewInt3 hitVoxel = VoxelUtils::coordToNewVoxel(coord);

			if (isTransitionToInterior)
			{
				const NewInt2 voxelXZ(hitVoxel.x, hitVoxel.z);
				const OriginalInt2 originalVoxel = VoxelUtils::newVoxelToOriginalVoxel(voxelXZ);
				const OriginalInt2 doorVoxel = [activeMapType, &originalVoxel]()
				{
					if (activeMapType == MapType::City)
					{
						return originalVoxel;
					}
					else if (activeMapType == MapType::Wilderness)
					{
						// Get the door voxel using the relative wilderness origin near the player
						// as the reference.
						const OriginalInt2 relativeOrigin = ArenaWildUtils::getRelativeWildOrigin(originalVoxel);
						const OriginalInt2 relativeVoxel = originalVoxel - relativeOrigin;
						return relativeVoxel;
					}
					else
					{
						DebugUnhandledReturnMsg(OriginalInt2, std::to_string(static_cast<int>(activeMapType)));
					}
				}();

				const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
				const auto &exeData = binaryAssetLibrary.getExeData();
				const std::string mifName = ArenaLevelUtils::getDoorVoxelMifName(doorVoxel.x, doorVoxel.y,
					menuID, cityDef.rulerSeed, cityDef.palaceIsMainQuestDungeon, cityDef.type,
					activeMapType, exeData);

				// @todo: the return data needs to include chunk coordinates when in the
				// wilderness. Maybe make that a discriminated union: "city return" and
				// "wild return".
				const NewInt3 returnVoxel = [&voxelHit, &hitVoxel]()
				{
					const NewInt3 delta = [&voxelHit, &hitVoxel]()
					{
						// Assuming this is a wall voxel.
						DebugAssert(voxelHit.facing.has_value());
						const VoxelFacing3D facing = *voxelHit.facing;

						if (facing == VoxelFacing3D::PositiveX)
						{
							return NewInt3(1, 0, 0);
						}
						else if (facing == VoxelFacing3D::NegativeX)
						{
							return NewInt3(-1, 0, 0);
						}
						else if (facing == VoxelFacing3D::PositiveZ)
						{
							return NewInt3(0, 0, 1);
						}
						else if (facing == VoxelFacing3D::NegativeZ)
						{
							return NewInt3(0, 0, -1);
						}
						else
						{
							DebugUnhandledReturnMsg(NewInt3, std::to_string(static_cast<int>(facing)));
						}
					}();

					return hitVoxel + delta;
				}();

				// Enter the interior location if the .MIF name is valid.
				if (mifName.size() > 0)
				{
					// @todo: I think dungeons can't use enterInterior(). They need an enterDungeon() method.
					MIFFile mif;
					if (!mif.init(mifName.c_str()))
					{
						DebugCrash("Could not init .MIF file \"" + mifName + "\".");
					}

					const std::optional<ArenaTypes::InteriorType> interiorType =
						ArenaInteriorUtils::menuTypeToInteriorType(menuType);
					DebugAssert(interiorType.has_value());
					gameData.enterInterior(*interiorType, mif, NewInt2(returnVoxel.x, returnVoxel.z),
						game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
						binaryAssetLibrary, game.getRandom(), game.getTextureManager(),
						game.getRenderer());

					// Change to interior music.
					const MusicLibrary &musicLibrary = game.getMusicLibrary();
					const MusicDefinition *musicDef = nullptr;
					MusicDefinition::InteriorMusicDefinition::Type interiorMusicType;
					if (MusicUtils::tryGetInteriorMusicType(mifName, &interiorMusicType))
					{
						// Non-dungeon interior.
						musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Interior,
							game.getRandom(), [interiorMusicType](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Interior);
							const auto &interiorMusicDef = def.getInteriorMusicDefinition();
							return interiorMusicDef.type == interiorMusicType;
						});
					}
					else
					{
						// Dungeon.
						musicDef = musicLibrary.getRandomMusicDefinition(
							MusicDefinition::Type::Dungeon, game.getRandom());
					}

					if (musicDef == nullptr)
					{
						DebugLogWarning("Missing interior music.");
					}

					AudioManager &audioManager = game.getAudioManager();
					audioManager.setMusic(musicDef);
				}
				else
				{
					// @todo: handle wilderness dungeon .MIF names differently than just with
					// an empty string?
					DebugLogWarning("Empty .MIF name at voxel (" + std::to_string(voxelXZ.x) + ", " +
						std::to_string(voxelXZ.y) + ").");
				}
			}
			else
			{
				// City gate transition.
				const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
				const ProvinceDefinition &provinceDef = gameData.getProvinceDefinition();
				const LocationDefinition &locationDef = gameData.getLocationDefinition();
				const int starCount = SkyUtils::getStarCountFromDensity(
					game.getOptions().getMisc_StarDensity());

				if (activeMapType == MapType::City)
				{
					// From city to wilderness. Use the gate position to determine where to put the
					// player in the wilderness.
					const NewInt2 gatePos(hitVoxel.x, hitVoxel.z);
					const NewInt2 transitionDir = [&voxelHit]()
					{
						// Assuming this is a wall voxel.
						DebugAssert(voxelHit.facing.has_value());
						const VoxelFacing3D facing = *voxelHit.facing;

						if (facing == VoxelFacing3D::PositiveX)
						{
							return VoxelUtils::North;
						}
						else if (facing == VoxelFacing3D::NegativeX)
						{
							return VoxelUtils::South;
						}
						else if (facing == VoxelFacing3D::PositiveZ)
						{
							return VoxelUtils::East;
						}
						else if (facing == VoxelFacing3D::NegativeZ)
						{
							return VoxelUtils::West;
						}
						else
						{
							DebugUnhandledReturnMsg(NewInt2, std::to_string(static_cast<int>(facing)));
						}
					}();

					const bool ignoreGatePos = false;
					if (!gameData.loadWilderness(locationDef, provinceDef, gatePos, transitionDir,
						ignoreGatePos, gameData.getWeatherType(), starCount,
						game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
						binaryAssetLibrary, game.getRandom(), textureManager, renderer))
					{
						DebugCrash("Couldn't load wilderness \"" + locationDef.getName() + "\".");
					}
				}
				else if (activeMapType == MapType::Wilderness)
				{
					// From wilderness to city.
					if (!gameData.loadCity(locationDef, provinceDef, gameData.getWeatherType(),
						starCount, game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
						binaryAssetLibrary, game.getTextAssetLibrary(), game.getRandom(), textureManager,
						renderer))
					{
						DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
					}
				}
				else
				{
					DebugLogError("World type \"" + std::to_string(static_cast<int>(activeMapType)) +
						"\" does not support city gate transitions.");
					return;
				}

				// Reset the current music (even if it's the same one).
				const MusicLibrary &musicLibrary = game.getMusicLibrary();
				const MusicDefinition *musicDef = [&game, &gameData, &locationDef, &musicLibrary]()
				{
					const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
					const ClimateType climateType = cityDef.climateType;
					const WeatherType filteredWeatherType = WeatherUtils::getFilteredWeatherType(
						gameData.getWeatherType(), climateType);

					if (!gameData.nightMusicIsActive())
					{
						return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
							game.getRandom(), [filteredWeatherType](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Weather);
							const auto &weatherMusicDef = def.getWeatherMusicDefinition();
							return weatherMusicDef.type == filteredWeatherType;
						});
					}
					else
					{
						return musicLibrary.getRandomMusicDefinition(
							MusicDefinition::Type::Night, game.getRandom());
					}
				}();

				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing exterior music.");
				}

				// Only play jingle when going wilderness to city.
				const MusicDefinition *jingleMusicDef = nullptr;
				if (activeMapType == MapType::Wilderness)
				{
					jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
						game.getRandom(), [&cityDef](const MusicDefinition &def)
					{
						DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
						const auto &jingleMusicDef = def.getJingleMusicDefinition();
						return (jingleMusicDef.cityType == cityDef.type) &&
							(jingleMusicDef.climateType == cityDef.climateType);
					});

					if (jingleMusicDef == nullptr)
					{
						DebugLogWarning("Missing jingle music.");
					}
				}

				AudioManager &audioManager = game.getAudioManager();
				audioManager.setMusic(musicDef, jingleMusicDef);
			}
		}
	}
}

void GameWorldPanel::handleLevelTransition(const NewInt2 &playerVoxel, const NewInt2 &transitionVoxel)
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();

	// Level transitions are always between interiors.
	auto &interior = [&gameData]() -> WorldData&
	{
		auto &worldData = gameData.getActiveWorld();
		DebugAssert(worldData.getMapType() == MapType::Interior);
		return worldData;
	}();

	const auto &level = interior.getActiveLevel();
	const auto &voxelGrid = level.getVoxelGrid();

	// Get the voxel definition associated with the voxel.
	const auto &voxelDef = [&transitionVoxel, &voxelGrid]()
	{
		const SNInt x = transitionVoxel.x;
		const int y = 1;
		const WEInt z = transitionVoxel.y;
		const uint16_t voxelID = voxelGrid.getVoxel(x, y, z);
		return voxelGrid.getVoxelDef(voxelID);
	}();

	// If the associated voxel data is a wall, then it might be a transition voxel.
	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const LevelData::Transitions &transitions = level.getTransitions();
		const auto transitionIter = transitions.find(transitionVoxel);
		if (transitionIter != transitions.end())
		{
			// The direction from a level up/down voxel to where the player should end up after
			// going through. In other words, it points to the destination voxel adjacent to the
			// level up/down voxel.
			auto dirToNewVoxel = [&playerVoxel, &transitionVoxel]()
			{
				const SNInt diffX = transitionVoxel.x - playerVoxel.x;
				const WEInt diffZ = transitionVoxel.y - playerVoxel.y;

				// @todo: this probably isn't robust enough. Maybe also check the player's angle
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
					throw DebugException("Bad player transition voxel.");
				}
			}();

			// Player destination after going through a level up/down voxel.
			auto &player = gameData.getPlayer();
			const NewDouble2 transitionVoxelCenter = VoxelUtils::getVoxelCenter(transitionVoxel);
			const NewDouble2 destinationXZ(
				transitionVoxelCenter.x + dirToNewVoxel.x,
				transitionVoxelCenter.y + dirToNewVoxel.z);

			// Lambda for transitioning the player to the given level.
			auto switchToLevel = [&game, &gameData, &interior, &player, &destinationXZ,
				&dirToNewVoxel](int levelIndex)
			{
				// Clear all open doors and fading voxels in the level the player is switching away from.
				// @todo: why wouldn't it just clear them when it gets switched to in setActive()?
				auto &oldActiveLevel = interior.getActiveLevel();
				oldActiveLevel.clearTemporaryVoxelInstances();

				// Select the new level.
				interior.setActiveLevelIndex(levelIndex);

				// Set the new level active in the renderer.
				auto &newActiveLevel = interior.getActiveLevel();
				newActiveLevel.setActive(gameData.nightLightsAreActive(), interior,
					gameData.getProvinceDefinition(), gameData.getLocationDefinition(),
					game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
					game.getBinaryAssetLibrary(), game.getRandom(), gameData.getCitizenManager(),
					game.getTextureManager(), game.getRenderer());

				// Move the player to where they should be in the new level.
				const NewDouble3 playerDestinationPoint(
					destinationXZ.x,
					newActiveLevel.getCeilingHeight() + Player::HEIGHT,
					destinationXZ.y);
				const CoordDouble3 playerDestinationCoord = VoxelUtils::newPointToCoord(playerDestinationPoint);
				player.teleport(playerDestinationCoord);
				player.lookAt(player.getPosition() + dirToNewVoxel);
				player.setVelocityToZero();
			};

			// Lambda for opening the world map when the player enters a transition voxel
			// that will "lead to the surface of the dungeon".
			auto switchToWorldMap = [&playerVoxel, &game, &player]()
			{
				// Move player to center of previous voxel in case they change their mind
				// about fast traveling. Don't change their direction.
				const NewDouble3 playerDestinationPoint(
					static_cast<SNDouble>(playerVoxel.x) + 0.50,
					player.getPosition().point.y,
					static_cast<WEDouble>(playerVoxel.y) + 0.50);
				const CoordDouble3 playerDestinationCoord = VoxelUtils::newPointToCoord(playerDestinationPoint);
				player.teleport(playerDestinationCoord);
				player.setVelocityToZero();

				game.setPanel<WorldMapPanel>(game, nullptr);
			};

			// Check the voxel type to determine what it is exactly.
			const LevelData::Transition &transition = transitionIter->second;
			if (transition.getType() == LevelData::Transition::Type::Menu)
			{
				const LevelData::Transition::Menu &transitionMenu = transition.getMenu();
				DebugLog("Entered *MENU " + std::to_string(transitionMenu.id) + ".");
			}
			else if (transition.getType() == LevelData::Transition::Type::LevelUp)
			{
				// If the custom function has a target, call it and reset it.
				auto &onLevelUpVoxelEnter = gameData.getOnLevelUpVoxelEnter();

				if (onLevelUpVoxelEnter)
				{
					onLevelUpVoxelEnter(game);
					onLevelUpVoxelEnter = std::function<void(Game&)>();
				}
				else if (interior.getActiveLevelIndex() > 0)
				{
					// Decrement the world's level index and activate the new level.
					switchToLevel(interior.getActiveLevelIndex() - 1);
				}
				else
				{
					switchToWorldMap();
				}
			}
			else if (transition.getType() == LevelData::Transition::Type::LevelDown)
			{
				if (interior.getActiveLevelIndex() < (interior.getLevelCount() - 1))
				{
					// Increment the world's level index and activate the new level.
					switchToLevel(interior.getActiveLevelIndex() + 1);
				}
				else
				{
					switchToWorldMap();
				}
			}
		}
	}
}

void GameWorldPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip = Panel::createTooltip(
		text, FontName::D, this->getGame().getFontLibrary(), renderer);

	auto &textureManager = this->getGame().getTextureManager();
	const TextureBuilderID gameWorldInterfaceTextureBuilderID =
		GameWorldPanel::getGameWorldInterfaceTextureBuilderID(textureManager);
	const TextureBuilder &gameWorldInterfaceTextureBuilder =
		textureManager.getTextureBuilderHandle(gameWorldInterfaceTextureBuilderID);

	const int x = 0;
	const int y = ArenaRenderUtils::SCREEN_HEIGHT -
		gameWorldInterfaceTextureBuilder.getHeight() - tooltip.getHeight();
	renderer.drawOriginal(tooltip, x, y);
}

void GameWorldPanel::drawCompass(const NewDouble2 &direction, TextureManager &textureManager, Renderer &renderer)
{
	// The compass slider is drawn based on player direction.
	const TextureBuilderID compassSliderTextureBuilderID = this->getCompassSliderTextureBuilderID();
	const TextureBuilderID compassFrameTextureBuilderID = this->getCompassFrameTextureBuilderID();

	// Angle between 0 and 2 pi.
	const double angle = std::atan2(-direction.y, -direction.x);

	// Offset in the "slider" texture. Due to how SLIDER.IMG is drawn, there's a
	// small "pop-in" when turning from N to NE, because N is drawn in two places,
	// but the second place (offset == 256) has tick marks where "NE" should be.
	const int xOffset = static_cast<int>(240.0 +
		std::round(256.0 * (angle / (2.0 * Constants::Pi)))) % 256;

	// Clip area for the visible part of the slider.
	const TextureBuilder &compassSlider = textureManager.getTextureBuilderHandle(compassSliderTextureBuilderID);
	const Rect clipRect(xOffset, 0, 32, compassSlider.getHeight());

	// Top-left corner of the slider in 320x200 space.
	const int sliderX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (clipRect.getWidth() / 2);
	const int sliderY = clipRect.getHeight();

	// Since there are some off-by-one rounding errors with SDL_RenderCopy,
	// draw a black rectangle behind the slider to cover up gaps.
	renderer.fillOriginalRect(Color::Black, sliderX - 1, sliderY - 1,
		clipRect.getWidth() + 2, clipRect.getHeight() + 2);

	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return;
	}

	renderer.drawOriginalClipped(compassSliderTextureBuilderID, *paletteID, clipRect,
		sliderX, sliderY, textureManager);

	// Draw the compass frame over the slider.
	const TextureBuilder &compassFrame = textureManager.getTextureBuilderHandle(compassFrameTextureBuilderID);
	renderer.drawOriginal(compassFrameTextureBuilderID, *paletteID,
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (compassFrame.getWidth() / 2), 0, textureManager);
}

void GameWorldPanel::drawProfiler(int profilerLevel, Renderer &renderer)
{
	DebugAssert(profilerLevel > Options::MIN_PROFILER_LEVEL);
	DebugAssert(profilerLevel <= Options::MAX_PROFILER_LEVEL);

	auto &game = this->getGame();

	const FPSCounter &fpsCounter = game.getFPSCounter();
	const double fps = fpsCounter.getAverageFPS();
	const double frameTimeMS = 1000.0 / fps;

	const auto &options = game.getOptions();
	const int targetFps = options.getGraphics_TargetFPS();
	const int minFps = Options::MIN_FPS;

	// Draw each profiler level with its own draw call.
	if (profilerLevel >= 1)
	{
		// FPS.
		const std::string fpsText = String::fixedPrecision(fps, 1);
		const std::string frameTimeText = String::fixedPrecision(frameTimeMS, 1);
		const std::string text = "FPS: " + fpsText + " (" + frameTimeText + "ms)";

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::D,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		const int x = 2;
		const int y = 2;
		const TextBox textBox(x, y, richText, fontLibrary, renderer);
		renderer.drawOriginal(textBox.getTexture(), x, y);
	}

	if (profilerLevel >= 2)
	{
		// Screen, renderer, timing, and player info.
		const Int2 windowDims = renderer.getWindowDimensions();

		const Renderer::ProfilerData &profilerData = renderer.getProfilerData();
		const Int2 renderDims(profilerData.width, profilerData.height);
		const double resolutionScale = options.getGraphics_ResolutionScale();

		auto &gameData = game.getGameData();
		const auto &player = gameData.getPlayer();
		const NewDouble3 absolutePosition = VoxelUtils::coordToNewPoint(player.getPosition());
		const Double3 &direction = player.getDirection();

		const std::string windowWidth = std::to_string(windowDims.x);
		const std::string windowHeight = std::to_string(windowDims.y);

		const std::string renderWidth = std::to_string(renderDims.x);
		const std::string renderHeight = std::to_string(renderDims.y);
		const std::string renderResScale = String::fixedPrecision(resolutionScale, 2);
		const std::string renderThreadCount = std::to_string(profilerData.threadCount);

		const std::string posX = String::fixedPrecision(absolutePosition.x, 2);
		const std::string posY = String::fixedPrecision(absolutePosition.y, 2);
		const std::string posZ = String::fixedPrecision(absolutePosition.z, 2);
		const std::string dirX = String::fixedPrecision(direction.x, 2);
		const std::string dirY = String::fixedPrecision(direction.y, 2);
		const std::string dirZ = String::fixedPrecision(direction.z, 2);

		std::string text =
			"Screen: " + windowWidth + "x" + windowHeight + '\n' +
			"Render: " + renderWidth + "x" + renderHeight + " (" + renderResScale + "), " +
			renderThreadCount + " thread" + ((profilerData.threadCount > 1) ? "s" : "") + '\n' +
			"Pos: " + posX + ", " + posY + ", " + posZ + '\n' +
			"Dir: " + dirX + ", " + dirY + ", " + dirZ;

		// Add any wilderness-specific info.
		const auto &worldData = game.getGameData().getActiveWorld();
		const MapType mapType = worldData.getMapType();
		if (mapType == MapType::Wilderness)
		{
			const auto &activeLevel = worldData.getActiveLevel();
			const auto &voxelGrid = activeLevel.getVoxelGrid();

			const NewInt3 playerVoxel = VoxelUtils::pointToVoxel(absolutePosition);
			const OriginalInt2 originalVoxel = VoxelUtils::newVoxelToOriginalVoxel(
				NewInt2(playerVoxel.x, playerVoxel.z));
			const Int2 chunkCoord(
				originalVoxel.x / RMDFile::WIDTH,
				originalVoxel.y / RMDFile::DEPTH);

			const std::string chunkX = std::to_string(chunkCoord.x);
			const std::string chunkY = std::to_string(chunkCoord.y);

			text += "\nChunk: " + chunkX + ", " + chunkY;
		}

		auto &fontLibrary = game.getFontLibrary();
		const FontName fontName = FontName::D;
		const RichTextString richText(
			text,
			fontName,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		// Get character height of the FPS font so Y position is correct.
		const char *fontNameStr = FontUtils::fromName(fontName);
		int fontIndex;
		if (!fontLibrary.tryGetDefinitionIndex(fontNameStr, &fontIndex))
		{
			DebugLogWarning("Couldn't get font \"" + std::string(fontNameStr) + "\".");
			return;
		}

		const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
		const int yOffset = fontDef.getCharacterHeight();

		const int x = 2;
		const int y = 2 + yOffset;
		const TextBox textBox(x, y, richText, fontLibrary, renderer);
		renderer.drawOriginal(textBox.getTexture(), x, y);
	}

	if (profilerLevel >= 3)
	{
		// Draw frame times and graph.
		const Renderer::ProfilerData &profilerData = renderer.getProfilerData();
		const std::string renderTime = String::fixedPrecision(profilerData.frameTime * 1000.0, 2);

		const std::string text =
			"3D render: " + renderTime + "ms" + "\n" +
			"Vis flats: " + std::to_string(profilerData.visFlatCount) + " (" +
			std::to_string(profilerData.potentiallyVisFlatCount) + ")" +
			", lights: " + std::to_string(profilerData.visLightCount) + "\n" +
			"FPS Graph:" + '\n' +
			"                               " + std::to_string(targetFps) + "\n\n\n\n" +
			"                               " + std::to_string(0);

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::D,
			Color::White,
			TextAlignment::Left,
			fontLibrary);

		const int x = 2;
		const int y = 72;
		const TextBox textBox(x, y, richText, fontLibrary, renderer);

		const Texture frameTimesGraph = [&renderer, &game, &fpsCounter, targetFps]()
		{
			// Graph maximum is target FPS, minimum is MIN_FPS.
			const int columnWidth = 1;
			const int width = fpsCounter.getFrameCount() * columnWidth;
			const int height = 32;
			Surface surface = Surface::createWithFormat(
				width, height, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			surface.fill(0, 0, 0, 128);

			const std::array<uint32_t, 3> Colors =
			{
				surface.mapRGBA(255, 0, 0, 128),
				surface.mapRGBA(255, 255, 0, 128),
				surface.mapRGBA(0, 255, 0, 128)
			};

			auto drawGraphColumn = [columnWidth, &surface, &Colors](int x, double percent)
			{
				const uint32_t color = [&Colors, percent]()
				{
					const int colorIndex = [percent]()
					{
						if (percent < (1.0 / 3.0))
						{
							return 0;
						}
						else if (percent < (2.0 / 3.0))
						{
							return 1;
						}
						else
						{
							return 2;
						}
					}();

					return Colors.at(colorIndex);
				}();

				// Height of column in pixels.
				const int height = std::clamp(static_cast<int>(
					percent * static_cast<double>(surface.getHeight())), 0, surface.getHeight());

				const Rect rect(x * columnWidth, surface.getHeight() - height, columnWidth, height);
				surface.fillRect(rect, color);
			};

			// Fill in columns.
			const double targetFpsReal = static_cast<double>(targetFps);
			for (int i = 0; i < fpsCounter.getFrameCount(); i++)
			{
				const double frameTime = fpsCounter.getFrameTime(i);
				const double fps = 1.0 / frameTime;
				const double fpsPercent = std::clamp(fps / targetFpsReal, 0.0, 1.0);
				drawGraphColumn(i, fpsPercent);
			}

			return renderer.createTextureFromSurface(surface);
		}();

		renderer.drawOriginal(textBox.getTexture(), textBox.getX(), textBox.getY());
		renderer.drawOriginal(frameTimesGraph, textBox.getX(), 94);
	}
}

void GameWorldPanel::tick(double dt)
{
	auto &game = this->getGame();
	DebugAssert(game.gameDataIsActive());

	// Get the relative mouse state.
	const auto &inputManager = game.getInputManager();
	const Int2 mouseDelta = inputManager.getMouseDelta();

	// Handle input for player motion.
	this->handlePlayerTurning(dt, mouseDelta);
	this->handlePlayerMovement(dt);

	// Tick the game world clock time.
	auto &gameData = game.getGameData();
	const bool debugFastForwardClock = inputManager.keyIsDown(SDL_SCANCODE_R); // @todo: camp button
	const Clock oldClock = gameData.getClock();
	gameData.tick(debugFastForwardClock ? (dt * 250.0) : dt, game);
	const Clock newClock = gameData.getClock();

	auto &renderer = game.getRenderer();

	// See if the clock passed the boundary between night and day, and vice versa.
	const double oldClockTime = oldClock.getPreciseTotalSeconds();
	const double newClockTime = newClock.getPreciseTotalSeconds();
	const double lamppostActivateTime = ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	const double lamppostDeactivateTime = ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool activateNightLights =
		(oldClockTime < lamppostActivateTime) &&
		(newClockTime >= lamppostActivateTime);
	const bool deactivateNightLights =
		(oldClockTime < lamppostDeactivateTime) &&
		(newClockTime >= lamppostDeactivateTime);

	if (activateNightLights)
	{
		this->handleNightLightChange(true);
	}
	else if (deactivateNightLights)
	{
		this->handleNightLightChange(false);
	}

	auto &worldData = gameData.getActiveWorld();
	const MapType mapType = worldData.getMapType();

	// Check for changes in exterior music depending on the time.
	if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		const double dayMusicStartTime = ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
		const double nightMusicStartTime = ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
		const bool changeToDayMusic =
			(oldClockTime < dayMusicStartTime) &&
			(newClockTime >= dayMusicStartTime);
		const bool changeToNightMusic =
			(oldClockTime < nightMusicStartTime) &&
			(newClockTime >= nightMusicStartTime);

		const MusicLibrary &musicLibrary = game.getMusicLibrary();

		if (changeToDayMusic)
		{
			const LocationDefinition &locationDef = gameData.getLocationDefinition();
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
			const WeatherType filteredWeatherType = WeatherUtils::getFilteredWeatherType(
				gameData.getWeatherType(), cityDef.climateType);

			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Weather, game.getRandom(),
				[filteredWeatherType](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Weather);
				const auto &weatherMusicDef = def.getWeatherMusicDefinition();
				return weatherMusicDef.type == filteredWeatherType;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing weather music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		}
		else if (changeToNightMusic)
		{
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
				MusicDefinition::Type::Night, game.getRandom());

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing night music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		}
	}

	// Tick the player.
	auto &player = gameData.getPlayer();
	const CoordDouble3 oldPlayerPoint = player.getPosition();
	player.tick(game, dt);
	const CoordDouble3 newPlayerPoint = player.getPosition();

	// Handle input for the player's attack.
	this->handlePlayerAttack(mouseDelta);

	// Handle door animations.
	const NewDouble3 newAbsolutePlayerPoint = VoxelUtils::coordToNewPoint(newPlayerPoint);
	const NewDouble2 newAbsolutePlayerPointXZ(newAbsolutePlayerPoint.x, newAbsolutePlayerPoint.z);
	this->handleDoors(dt, newAbsolutePlayerPointXZ);

	// Tick level data (entities, animated distant land, etc.).
	auto &levelData = worldData.getActiveLevel();
	levelData.tick(game, dt);

	// See if the player changed voxels in the XZ plane. If so, trigger text and
	// sound events, and handle any level transition.
	const NewDouble3 oldAbsolutePlayerPoint = VoxelUtils::coordToNewPoint(oldPlayerPoint);
	const NewInt3 oldAbsolutePlayerVoxel = VoxelUtils::pointToVoxel(oldAbsolutePlayerPoint);
	const NewInt3 newAbsolutePlayerVoxel = VoxelUtils::pointToVoxel(newAbsolutePlayerPoint);
	if ((newAbsolutePlayerVoxel.x != oldAbsolutePlayerVoxel.x) ||
		(newAbsolutePlayerVoxel.z != oldAbsolutePlayerVoxel.z))
	{
		const NewInt2 oldPlayerVoxelXZ(oldAbsolutePlayerVoxel.x, oldAbsolutePlayerVoxel.z);
		const NewInt2 newPlayerVoxelXZ(newAbsolutePlayerVoxel.x, newAbsolutePlayerVoxel.z);

		// Don't handle triggers and level transitions if outside the voxel grid.
		const bool inVoxelGrid = [&worldData, &newPlayerVoxelXZ]()
		{
			const auto &level = worldData.getActiveLevel();
			const auto &voxelGrid = level.getVoxelGrid();
			return (newPlayerVoxelXZ.x >= 0) && (newPlayerVoxelXZ.x < voxelGrid.getWidth()) &&
				(newPlayerVoxelXZ.y >= 0) && (newPlayerVoxelXZ.y < voxelGrid.getDepth());
		}();

		if (inVoxelGrid && (worldData.getMapType() == MapType::Interior))
		{
			this->handleTriggers(newPlayerVoxelXZ);

			// @todo: determine if the player would collide with the voxel instead
			// of checking that they're in the voxel.
			this->handleLevelTransition(oldPlayerVoxelXZ, newPlayerVoxelXZ);
		}
	}
}

void GameWorldPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Draw game world onto the native frame buffer. The game world buffer
	// might not completely fill up the native buffer (bottom corners), so
	// clearing the native buffer beforehand is still necessary.
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	auto &player = gameData.getPlayer();
	const auto &worldData = gameData.getActiveWorld();
	const auto &level = worldData.getActiveLevel();
	const auto &options = game.getOptions();
	const double ambientPercent = gameData.getAmbientPercent();

	const double latitude = [&gameData]()
	{
		const LocationDefinition &locationDef = gameData.getLocationDefinition();
		return locationDef.getLatitude();
	}();

	const bool isExterior = worldData.getMapType() != MapType::Interior;

	auto &textureManager = game.getTextureManager();
	const std::string &defaultPaletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> defaultPaletteID = textureManager.tryGetPaletteID(defaultPaletteFilename.c_str());
	if (!defaultPaletteID.has_value())
	{
		DebugLogError("Couldn't get default palette ID from \"" + defaultPaletteFilename + "\".");
		return;
	}

	const Palette &defaultPalette = textureManager.getPaletteHandle(*defaultPaletteID);

	renderer.renderWorld(player.getPosition(), player.getDirection(), options.getGraphics_VerticalFOV(),
		ambientPercent, gameData.getDaytimePercent(), gameData.getChasmAnimPercent(), latitude,
		gameData.nightLightsAreActive(), isExterior, options.getMisc_PlayerHasLight(),
		options.getMisc_ChunkDistance(), level.getCeilingHeight(), level, game.getEntityDefinitionLibrary(),
		defaultPalette);

	const TextureBuilderID gameWorldInterfaceTextureBuilderID =
		GameWorldPanel::getGameWorldInterfaceTextureBuilderID(textureManager);

	const TextureBuilderID statusGradientTextureBuilderID = [this]()
	{
		constexpr int gradientID = 0; // Default for now.
		return this->getStatusGradientTextureBuilderID(gradientID);
	}();
	
	const TextureBuilderID playerPortraitTextureBuilderID = [this, &player]()
	{
		const std::string &headsFilename = PortraitFile::getHeads(player.isMale(), player.getRaceID(), true);
		return this->getPlayerPortraitTextureBuilderID(headsFilename, player.getPortraitID());
	}();

	const TextureBuilderID noSpellTextureBuilderID = this->getNoSpellTextureBuilderID();

	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const bool modernInterface = options.getGraphics_ModernInterface();

	// Continue drawing more interface objects if in classic mode.
	// - @todo: clamp game world interface to screen edges, not letterbox edges.
	if (!modernInterface)
	{
		// Draw game world interface.
		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		renderer.drawOriginal(gameWorldInterfaceTextureBuilderID, *defaultPaletteID, 0,
			ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilderRef.getHeight(), textureManager);

		// Draw player portrait.
		renderer.drawOriginal(statusGradientTextureBuilderID, *defaultPaletteID, 14, 166, textureManager);
		renderer.drawOriginal(playerPortraitTextureBuilderID, *defaultPaletteID, 14, 166, textureManager);

		// If the player's class can't use magic, show the darkened spell icon.
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());
		if (!charClassDef.canCastMagic())
		{
			renderer.drawOriginal(noSpellTextureBuilderID, *defaultPaletteID, 91, 177, textureManager);
		}

		// Draw text: player name.
		renderer.drawOriginal(this->playerNameTextBox->getTexture(),
			this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	}
}

void GameWorldPanel::renderSecondary(Renderer &renderer)
{
	DebugAssert(this->getGame().gameDataIsActive());
	
	auto &gameData = this->getGame().getGameData();
	auto &player = gameData.getPlayer();
	const auto &options = this->getGame().getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	// Several interface objects are in this method because they are hidden by the status
	// pop-up and the spells list.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &defaultPaletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> defaultPaletteID = textureManager.tryGetPaletteID(defaultPaletteFilename.c_str());
	if (!defaultPaletteID.has_value())
	{
		DebugLogError("Couldn't get default palette ID from \"" + defaultPaletteFilename + "\".");
		return;
	}

	const TextureBuilderID gameWorldInterfaceTextureBuilderID =
		GameWorldPanel::getGameWorldInterfaceTextureBuilderID(textureManager);

	// Display player's weapon if unsheathed. The position also depends on whether
	// the interface is in classic or modern mode.
	const auto &weaponAnimation = player.getWeaponAnimation();
	if (!weaponAnimation.isSheathed())
	{
		const int weaponAnimIndex = weaponAnimation.getFrameIndex();
		const TextureBuilderID weaponTextureBuilderID = [this, &weaponAnimation, weaponAnimIndex]()
		{
			const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
			return this->getWeaponTextureBuilderID(weaponFilename, weaponAnimIndex);
		}();

		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		const TextureBuilderRef weaponTextureBuilderRef = textureManager.getTextureBuilderRef(weaponTextureBuilderID);

		DebugAssertIndex(this->weaponOffsets, weaponAnimIndex);
		const Int2 &weaponOffset = this->weaponOffsets[weaponAnimIndex];

		// Draw the current weapon image depending on interface mode.
		if (modernInterface)
		{
			// Draw stretched to fit the window.
			const int letterboxStretchMode = Options::MAX_LETTERBOX_MODE;
			renderer.setLetterboxMode(letterboxStretchMode);

			// Percent of the horizontal weapon offset across the original screen.
			const double weaponOffsetXPercent = static_cast<double>(weaponOffset.x) /
				static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);

			// Native left and right screen edges converted to original space.
			const int newLeft = renderer.nativeToOriginal(Int2(0, 0)).x;
			const int newRight = renderer.nativeToOriginal(
				Int2(renderer.getWindowDimensions().x, 0)).x;
			const double newDiff = static_cast<double>(newRight - newLeft);

			// Values to scale original weapon dimensions by.
			const double weaponScaleX = newDiff / static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
			const double weaponScaleY = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT) /
				static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilderRef.getHeight());

			const int weaponX = newLeft +
				static_cast<int>(std::round(newDiff * weaponOffsetXPercent));
			const int weaponY = static_cast<int>(std::round(
				static_cast<double>(weaponOffset.y) * weaponScaleY));
			const int weaponWidth = static_cast<int>(std::round(
				static_cast<double>(weaponTextureBuilderRef.getWidth()) * weaponScaleX));
			const int weaponHeight = static_cast<int>(std::round(
				static_cast<double>(std::min(weaponTextureBuilderRef.getHeight() + 1,
					std::max(ArenaRenderUtils::SCREEN_HEIGHT - weaponY, 0))) * weaponScaleY));

			renderer.drawOriginal(weaponTextureBuilderID, *defaultPaletteID,
				weaponX, weaponY, weaponWidth, weaponHeight, textureManager);

			// Reset letterbox mode back to what it was.
			renderer.setLetterboxMode(options.getGraphics_LetterboxMode());
		}
		else
		{
			// Clamp the max weapon height non-negative since some weapon animations like the
			// morning star can cause it to become -1.
			const int maxWeaponHeight = std::max(
				(ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilderRef.getHeight()) - weaponOffset.y, 0);

			// Add 1 to the height because Arena's renderer has an off-by-one bug, and a 1 pixel
			// gap appears unless a small change is added.
			const int weaponHeight = std::clamp(weaponTextureBuilderRef.getHeight() + 1, 0, maxWeaponHeight);
			renderer.drawOriginal(weaponTextureBuilderID, *defaultPaletteID,
				weaponOffset.x, weaponOffset.y, weaponTextureBuilderRef.getWidth(), weaponHeight, textureManager);
		}
	}

	// Draw the visible portion of the compass slider, and the frame over it.
	if (options.getMisc_ShowCompass())
	{
		this->drawCompass(player.getGroundDirection(), textureManager, renderer);
	}

	// Draw each pop-up text if its duration is positive.
	// - @todo: maybe give delta time to render()? Or store in tick()? I want to avoid
	//   subtracting the time in tick() because it would always be one frame shorter then.
	if (gameData.triggerTextIsVisible())
	{
		const Texture *triggerTextTexture;
		gameData.getTriggerTextRenderInfo(&triggerTextTexture);

		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		const int centerX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (triggerTextTexture->getWidth() / 2) - 1;
		const int centerY = [modernInterface, &gameWorldInterfaceTextureBuilderRef, triggerTextTexture]()
		{
			const int interfaceOffset = modernInterface ? (gameWorldInterfaceTextureBuilderRef.getHeight() / 2) :
				gameWorldInterfaceTextureBuilderRef.getHeight();
			return ArenaRenderUtils::SCREEN_HEIGHT - interfaceOffset - triggerTextTexture->getHeight() - 2;
		}();

		renderer.drawOriginal(*triggerTextTexture, centerX, centerY);
	}

	if (gameData.actionTextIsVisible())
	{
		const Texture *actionTextTexture;
		gameData.getActionTextRenderInfo(&actionTextTexture);

		const int textX = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (actionTextTexture->getWidth() / 2);
		const int textY = 20;
		renderer.drawOriginal(*actionTextTexture, textX, textY);
	}

	if (gameData.effectTextIsVisible())
	{
		// @todo: draw "effect text".
	}

	// Check if the mouse is over one of the buttons for tooltips in classic mode.
	if (!modernInterface)
	{
		auto &game = this->getGame();
		const auto &inputManager = game.getInputManager();
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

		// Get the hovered tooltip string, or the empty string if none are hovered over.
		const std::string tooltip = [this, &game, &player, &originalPosition]() -> std::string
		{
			const auto &charClassLibrary = game.getCharacterClassLibrary();
			const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());

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
			else if (this->magicButton.contains(originalPosition) && charClassDef.canCastMagic())
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

	// Draw some optional profiler text.
	const int profilerLevel = options.getMisc_ProfilerLevel();
	if (profilerLevel > Options::MIN_PROFILER_LEVEL)
	{
		this->drawProfiler(profilerLevel, renderer);
	}

	// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
	// levels where ceilingHeight < 1.0, and same with ceiling blue dots).
	if (profilerLevel == Options::MAX_PROFILER_LEVEL)
	{
		DEBUG_PhysicsRaycast(this->getGame(), renderer);
	}	
}
