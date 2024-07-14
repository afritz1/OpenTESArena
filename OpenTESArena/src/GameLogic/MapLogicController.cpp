#include "MapLogicController.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/Game.h"
#include "../Interface/WorldMapPanel.h"
#include "../Sky/SkyUtils.h"
#include "../UI/TextBox.h"
#include "../Voxels/VoxelFacing3D.h"
#include "../World/MapType.h"

void MapLogicController::handleNightLightChange(Game &game, bool active)
{
	SceneManager &sceneManager = game.getSceneManager();

	// Turn streetlights on or off.
	const std::string &newStreetlightAnimStateName = active ? EntityAnimationUtils::STATE_ACTIVATED : EntityAnimationUtils::STATE_IDLE;
	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	for (int i = 0; i < entityChunkManager.getChunkCount(); i++)
	{
		EntityChunk &entityChunk = entityChunkManager.getChunkAtIndex(i);
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			if (EntityUtils::isStreetlight(entityDef))
			{
				const EntityAnimationDefinition &entityAnimDef = entityDef.getAnimDef();
				const std::optional<int> newAnimStateIndex = entityAnimDef.tryGetStateIndex(newStreetlightAnimStateName.c_str());
				if (!newAnimStateIndex.has_value())
				{
					DebugLogError("Couldn't find \"" + newStreetlightAnimStateName + "\" animation state for streetlight entity \"" + std::to_string(entityInstID) + "\".");
					continue;
				}

				EntityAnimationInstance &entityAnimInst = entityChunkManager.getEntityAnimationInstance(entityInst.animInstID);
				entityAnimInst.setStateIndex(*newAnimStateIndex);				
			}
		}
	}

	const double ceilingScale = game.getGameState().getActiveCeilingScale();	
	RenderLightChunkManager &renderLightChunkManager = sceneManager.renderLightChunkManager;
	renderLightChunkManager.setNightLightsActive(active, ceilingScale, entityChunkManager);
}

void MapLogicController::handleTriggers(Game &game, const CoordInt3 &coord, TextBox &triggerTextBox)
{
	GameState &gameState = game.getGameState();
	SceneManager &sceneManager = game.getSceneManager();
	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	VoxelChunk *chunkPtr = voxelChunkManager.tryGetChunkAtPosition(coord.chunk);
	if (chunkPtr == nullptr)
	{
		DebugLogError("No voxel chunk at (" + coord.chunk.toString() + ") for checking triggers.");
		return;
	}

	VoxelChunk &chunk = *chunkPtr;
	const VoxelInt3 &voxel = coord.voxel;
	VoxelChunk::TriggerDefID triggerDefID;
	if (!chunk.tryGetTriggerDefID(voxel.x, voxel.y, voxel.z, &triggerDefID))
	{
		return;
	}

	const VoxelTriggerDefinition &triggerDef = chunk.getTriggerDef(triggerDefID);
	if (triggerDef.hasSoundDef())
	{
		const VoxelTriggerDefinition::SoundDef &soundDef = triggerDef.getSoundDef();
		const std::string &soundFilename = soundDef.getFilename();

		// Play the sound.
		auto &audioManager = game.getAudioManager();
		audioManager.playSound(soundFilename);
	}

	if (triggerDef.hasTextDef())
	{
		const VoxelTriggerDefinition::TextDef &textDef = triggerDef.getTextDef();
		const VoxelInt3 &voxel = coord.voxel;

		int triggerInstIndex;
		const bool hasBeenTriggered = chunk.tryGetTriggerInstIndex(voxel.x, voxel.y, voxel.z, &triggerInstIndex);
		const bool canDisplay = !textDef.isDisplayedOnce() || !hasBeenTriggered;

		if (canDisplay)
		{
			// Ignore the newline at the end.
			const std::string &textDefText = textDef.getText();
			const std::string text = textDefText.substr(0, textDefText.size() - 1);
			triggerTextBox.setText(text);
			gameState.setTriggerTextDuration(text);

			// Set the text trigger as activated regardless of whether it's single-shot, just for consistency.
			if (!hasBeenTriggered)
			{
				VoxelTriggerInstance newTriggerInst;
				newTriggerInst.init(voxel.x, voxel.y, voxel.z);
				chunk.addTriggerInst(std::move(newTriggerInst));
			}
		}
	}
}

void MapLogicController::handleMapTransition(Game &game, const Physics::Hit &hit, const TransitionDefinition &transitionDef)
{
	const TransitionType transitionType = transitionDef.getType();
	DebugAssert(transitionType != TransitionType::LevelChange);

	DebugAssert(hit.getType() == Physics::HitType::Voxel);
	const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
	const CoordInt3 hitCoord(hit.getCoord().chunk, voxelHit.voxel);

	auto &gameState = game.getGameState();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();

	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	DebugAssert(locationDef.getType() == LocationDefinitionType::City);
	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	// Decide based on the active world type.
	if (activeMapType == MapType::Interior)
	{
		DebugAssert(transitionType == TransitionType::ExitInterior);

		GameState::SceneChangeMusicFunc musicDefFunc = [](Game &game)
		{
			// Change to exterior music.
			GameState &gameState = game.getGameState();
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
			const Clock &clock = gameState.getClock();

			const MusicDefinition *musicDef = nullptr;
			if (!ArenaClockUtils::nightMusicIsActive(clock))
			{
				const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
				musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [&weatherDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.weatherDef == weatherDef;
				});
			}
			else
			{
				musicDef = musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::Night, game.getRandom());
			}

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing exterior music.");
			}

			return musicDef;
		};

		GameState::SceneChangeMusicFunc jingleMusicDefFunc = [](Game &game)
		{
			// Only play jingle if the exterior is inside the city walls.
			GameState &gameState = game.getGameState();
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();

			const MusicDefinition *jingleMusicDef = nullptr;
			if (gameState.getActiveMapDef().getMapType() == MapType::City)
			{
				const LocationDefinition &locationDef = gameState.getLocationDefinition();
				const LocationCityDefinition &locationCityDef = locationDef.getCityDefinition();
				jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
					game.getRandom(), [&locationCityDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
					const auto &jingleMusicDef = def.getJingleMusicDefinition();
					return (jingleMusicDef.cityType == locationCityDef.type) && (jingleMusicDef.climateType == locationCityDef.climateType);
				});

				if (jingleMusicDef == nullptr)
				{
					DebugLogWarning("Missing jingle music.");
				}
			}

			return jingleMusicDef;
		};

		// Leave the interior and go to the saved exterior.
		gameState.queueMapDefPop();
		gameState.queueMusicOnSceneChange(musicDefFunc, jingleMusicDefFunc);
	}
	else
	{
		// Either city or wilderness. If the transition is for an interior, enter it. If it's the city gates,
		// toggle between city and wilderness.
		if (transitionType == TransitionType::EnterInterior)
		{
			const CoordInt3 returnCoord = [&voxelHit, &hitCoord]()
			{
				const VoxelInt3 delta = [&voxelHit, &hitCoord]()
				{
					// Assuming this is a wall voxel.
					DebugAssert(voxelHit.facing.has_value());
					const VoxelFacing3D facing = *voxelHit.facing;

					if (facing == VoxelFacing3D::PositiveX)
					{
						return VoxelInt3(1, 0, 0);
					}
					else if (facing == VoxelFacing3D::NegativeX)
					{
						return VoxelInt3(-1, 0, 0);
					}
					else if (facing == VoxelFacing3D::PositiveZ)
					{
						return VoxelInt3(0, 0, 1);
					}
					else if (facing == VoxelFacing3D::NegativeZ)
					{
						return VoxelInt3(0, 0, -1);
					}
					else
					{
						DebugUnhandledReturnMsg(VoxelInt3, std::to_string(static_cast<int>(facing)));
					}
				}();

				return hitCoord + delta;
			}();

			const TransitionDefinition::InteriorEntranceDef &interiorEntranceDef = transitionDef.getInteriorEntrance();
			const MapGeneration::InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;

			MapDefinition mapDefinition;
			if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
			{
				DebugLogError("Couldn't init MapDefinition for interior type " + std::to_string(static_cast<int>(interiorGenInfo.getInteriorType())) + ".");
				return;
			}

			GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
			{
				// Change to interior music.
				const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
				const MapDefinition &activeMapDef = game.getGameState().getActiveMapDef();
				DebugAssert(activeMapDef.getMapType() == MapType::Interior);
				const MapDefinitionInterior &mapDefInterior = activeMapDef.getSubDefinition().interior;
				const ArenaTypes::InteriorType interiorType = mapDefInterior.interiorType;
				const MusicDefinition::InteriorMusicDefinition::Type interiorMusicType = MusicUtils::getInteriorMusicType(interiorType);
				const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
					MusicDefinition::Type::Interior, game.getRandom(), [interiorMusicType](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Interior);
					const auto &interiorMusicDef = def.getInteriorMusicDefinition();
					return interiorMusicDef.type == interiorMusicType;
				});

				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing interior music.");
				}

				return musicDef;
			};

			// Always use clear weather in interiors.
			WeatherDefinition overrideWeather;
			overrideWeather.initClear();

			gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, returnCoord, VoxelInt2::Zero, std::nullopt, false, overrideWeather);
			gameState.queueMusicOnSceneChange(musicFunc);
		}
		else if (transitionType == TransitionType::CityGate)
		{
			// City gate transition.
			const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
			const LocationDefinition &locationDef = gameState.getLocationDefinition();
			const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
			const int currentDay = gameState.getDate().getDay();
			const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());

			if (activeMapType == MapType::City)
			{
				// From city to wilderness. Use the gate position to determine where to put the player.

				// The voxel face that was hit determines where to put the player relative to the gate.
				const VoxelInt2 transitionDir = [&voxelHit]()
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
						DebugUnhandledReturnMsg(VoxelInt2, std::to_string(static_cast<int>(facing)));
					}
				}();

				const auto &exeData = binaryAssetLibrary.getExeData();
				Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs =
					ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

				MapGeneration::WildGenInfo wildGenInfo;
				wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// Calculate wilderness position based on the gate's voxel in the city.
				const CoordInt2 startCoord = [&hitCoord, &transitionDir]()
				{
					// Origin of the city in the wilderness.
					const ChunkInt2 wildCityChunk(ArenaWildUtils::CITY_ORIGIN_CHUNK_X, ArenaWildUtils::CITY_ORIGIN_CHUNK_Z);

					// Player position bias based on selected gate face.
					const VoxelInt2 offset(transitionDir.x, transitionDir.y);

					return CoordInt2(wildCityChunk + hitCoord.chunk, VoxelInt2(hitCoord.voxel.x, hitCoord.voxel.z) + offset);
				}();

				// No need to change world map location here.
				const std::optional<GameState::WorldMapLocationIDs> worldMapLocationIDs;

				MapDefinition mapDefinition;
				if (!mapDefinition.initWild(wildGenInfo, skyGenInfo, textureManager))
				{
					DebugLogError("Couldn't init MapDefinition for switch from city to wilderness for \"" + locationDef.getName() + "\".");
					return;
				}

				gameState.queueMapDefChange(std::move(mapDefinition), startCoord, std::nullopt, VoxelInt2::Zero, std::nullopt, true);
			}
			else if (activeMapType == MapType::Wilderness)
			{
				// From wilderness to city.
				Buffer<uint8_t> reservedBlocks = [&cityDef]()
				{
					DebugAssert(cityDef.reservedBlocks != nullptr);
					Buffer<uint8_t> buffer(static_cast<int>(cityDef.reservedBlocks->size()));
					std::copy(cityDef.reservedBlocks->begin(), cityDef.reservedBlocks->end(), buffer.begin());
					return buffer;
				}();

				const std::optional<LocationCityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
					[&cityDef]() -> std::optional<LocationCityDefinition::MainQuestTempleOverride>
				{
					if (cityDef.hasMainQuestTempleOverride)
					{
						return cityDef.mainQuestTempleOverride;
					}
					else
					{
						return std::nullopt;
					}
				}();

				MapGeneration::CityGenInfo cityGenInfo;
				cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
					cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
					cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
					mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY,
					cityDef.cityBlocksPerSide);

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// No need to change world map location here.
				const std::optional<GameState::WorldMapLocationIDs> worldMapLocationIDs;

				MapDefinition mapDefinition;
				if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, textureManager))
				{
					DebugLogError("Couldn't init MapDefinition for switch from wilderness to city for \"" + locationDef.getName() + "\".");
					return;
				}

				gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, std::nullopt, true);
			}
			else
			{
				DebugLogError("Map type \"" + std::to_string(static_cast<int>(activeMapType)) +
					"\" does not support city gate transitions.");
				return;
			}

			// Reset the current music (even if it's the same one).
			GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
			{
				const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
				const MusicDefinition *musicDef = [&game, &musicLibrary]()
				{
					GameState &gameState = game.getGameState();
					const Clock &clock = gameState.getClock();
					if (!ArenaClockUtils::nightMusicIsActive(clock))
					{
						const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
						return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
							game.getRandom(), [&weatherDef](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Weather);
							const auto &weatherMusicDef = def.getWeatherMusicDefinition();
							return weatherMusicDef.weatherDef == weatherDef;
						});
					}
					else
					{
						return musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::Night, game.getRandom());
					}
				}();

				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing exterior music.");
				}

				return musicDef;
			};

			const ArenaTypes::CityType cityDefType = cityDef.type;
			const ArenaTypes::ClimateType cityDefClimateType = cityDef.climateType;
			GameState::SceneChangeMusicFunc jingleMusicFunc = [cityDefType, cityDefClimateType](Game &game)
			{
				// Only play jingle when going wilderness to city.
				GameState &gameState = game.getGameState();
				const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
				const MapDefinition &activeMapDef = gameState.getActiveMapDef();
				const MusicDefinition *jingleMusicDef = nullptr;
				if (activeMapDef.getMapType() == MapType::City)
				{
					jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
						game.getRandom(), [cityDefType, cityDefClimateType](const MusicDefinition &def)
					{
						DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
						const auto &jingleMusicDef = def.getJingleMusicDefinition();
						return (jingleMusicDef.cityType == cityDefType) && (jingleMusicDef.climateType == cityDefClimateType);
					});

					if (jingleMusicDef == nullptr)
					{
						DebugLogWarning("Missing jingle music.");
					}
				}

				return jingleMusicDef;
			};

			gameState.queueMusicOnSceneChange(musicFunc, jingleMusicFunc);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(transitionType)));
		}
	}
}

void MapLogicController::handleLevelTransition(Game &game, const CoordInt3 &playerCoord,
	const CoordInt3 &transitionCoord)
{
	auto &gameState = game.getGameState();

	// Level transitions are always between interiors.
	const MapDefinition &interiorMapDef = gameState.getActiveMapDef();
	DebugAssert(interiorMapDef.getMapType() == MapType::Interior);

	const SceneManager &sceneManager = game.getSceneManager();
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const VoxelChunk *chunkPtr = voxelChunkManager.tryGetChunkAtPosition(transitionCoord.chunk);
	if (chunkPtr == nullptr)
	{
		DebugLogError("No voxel chunk at (" + transitionCoord.chunk.toString() + ") for checking level transition.");
		return;
	}

	const VoxelChunk &chunk = *chunkPtr;
	const VoxelInt3 &transitionVoxel = transitionCoord.voxel;
	if (!chunk.isValidVoxel(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z))
	{
		// Not in the chunk.
		return;
	}

	// Get the voxel definition associated with the voxel.
	const VoxelTraitsDefinition &voxelTraitsDef = [&chunk, &transitionVoxel]()
	{
		const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z);
		return chunk.getTraitsDef(voxelTraitsDefID);
	}();

	// If the associated voxel data is a wall, then it might be a transition voxel.
	if (voxelTraitsDef.type == ArenaTypes::VoxelType::Wall)
	{
		const VoxelInt3 &voxel = transitionCoord.voxel;
		VoxelChunk::TransitionDefID transitionDefID;
		if (!chunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
		{
			return;
		}

		const TransitionDefinition &transitionDef = chunk.getTransitionDef(transitionDefID);

		// The direction from a level up/down voxel to where the player should end up after
		// going through. In other words, it points to the destination voxel adjacent to the
		// level up/down voxel.
		const VoxelInt3 dirToWorldVoxel = [&playerCoord, &transitionCoord]()
		{
			const VoxelInt3 diff = transitionCoord - playerCoord;

			// @todo: this probably isn't robust enough. Maybe also check the player's angle
			// of velocity with angles to the voxel's corners to get the "arrival vector"
			// and thus the "near face" that is intersected, because this method doesn't
			// handle the player coming in at a diagonal.

			// Check which way the player is going and get the reverse of it.
			if (diff.x > 0)
			{
				// From south to north.
				return -Int3::UnitX;
			}
			else if (diff.x < 0)
			{
				// From north to south.
				return Int3::UnitX;
			}
			else if (diff.z > 0)
			{
				// From west to east.
				return -Int3::UnitZ;
			}
			else if (diff.z < 0)
			{
				// From east to west.
				return Int3::UnitZ;
			}
			else
			{
				throw DebugException("Bad player transition voxel.");
			}
		}();

		// Player destination after going through a level up/down voxel.
		auto &player = game.getPlayer();
		const VoxelDouble3 transitionVoxelCenter = VoxelUtils::getVoxelCenter(transitionCoord.voxel);
		const VoxelInt2 dirToWorldVoxelXZ(dirToWorldVoxel.x, dirToWorldVoxel.z);
		const VoxelDouble3 dirToWorldPoint(
			static_cast<SNDouble>(dirToWorldVoxel.x),
			static_cast<double>(dirToWorldVoxel.y),
			static_cast<WEDouble>(dirToWorldVoxel.z));
		const CoordDouble3 destinationCoord = ChunkUtils::recalculateCoord(transitionCoord.chunk, transitionVoxelCenter + dirToWorldPoint);

		// Lambda for opening the world map when the player enters a transition voxel
		// that will "lead to the surface of the dungeon".
		auto switchToWorldMap = [&playerCoord, &game, &player]()
		{
			// Move player to center of previous voxel in case they change their mind
			// about fast traveling. Don't change their direction.
			const VoxelInt2 playerVoxelXZ(playerCoord.voxel.x, playerCoord.voxel.z);
			const VoxelDouble2 playerVoxelCenterXZ = VoxelUtils::getVoxelCenter(playerVoxelXZ);
			const VoxelDouble3 playerDestinationPoint(
				playerVoxelCenterXZ.x,
				player.getPosition().point.y,
				playerVoxelCenterXZ.y);
			const CoordDouble3 playerDestinationCoord(playerCoord.chunk, playerDestinationPoint);
			player.teleport(playerDestinationCoord);
			player.setVelocityToZero();

			game.setPanel<WorldMapPanel>();
		};

		// See if it's a level up or level down transition. Ignore other transition types.
		if (transitionDef.getType() == TransitionType::LevelChange)
		{
			const int activeLevelIndex = gameState.getActiveLevelIndex();
			const int levelCount = interiorMapDef.getLevels().getCount();
			const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef.getLevelChange();
			if (levelChangeDef.isLevelUp)
			{
				// Level up transition. If the custom function has a target, call it and reset it (necessary
				// for main quest start dungeon).
				auto &onLevelUpVoxelEnter = gameState.getOnLevelUpVoxelEnter();

				if (onLevelUpVoxelEnter)
				{
					onLevelUpVoxelEnter(game);
					onLevelUpVoxelEnter = std::function<void(Game&)>();
				}
				else if (activeLevelIndex > 0)
				{
					// Decrement the world's level index and activate the new level.
					gameState.queueLevelIndexChange(activeLevelIndex - 1, dirToWorldVoxelXZ);
				}
				else
				{
					switchToWorldMap();
				}
			}
			else
			{
				// Level down transition.
				if (activeLevelIndex < (levelCount - 1))
				{
					// Increment the world's level index and activate the new level.
					gameState.queueLevelIndexChange(activeLevelIndex + 1, dirToWorldVoxelXZ);
				}
				else
				{
					switchToWorldMap();
				}
			}
		}
	}
}
