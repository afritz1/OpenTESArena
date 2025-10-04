#include "MapLogic.h"
#include "MapType.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Collision/RayCastTypes.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Interface/GameWorldUiController.h"
#include "../Interface/WorldMapPanel.h"
#include "../Sky/SkyUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../UI/TextBox.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../Voxels/VoxelFacing.h"

void MapLogic::handleNightLightChange(Game &game, bool active)
{
	SceneManager &sceneManager = game.sceneManager;

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
				const EntityAnimationDefinition &entityAnimDef = entityDef.animDef;
				const std::optional<int> newAnimStateIndex = entityAnimDef.findStateIndex(newStreetlightAnimStateName.c_str());
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
}

void MapLogic::handleTriggersInVoxel(Game &game, const CoordInt3 &coord, TextBox &triggerTextBox)
{
	GameState &gameState = game.gameState;
	SceneManager &sceneManager = game.sceneManager;
	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	VoxelChunk *chunkPtr = voxelChunkManager.findChunkAtPosition(coord.chunk);
	if (chunkPtr == nullptr)
	{
		DebugLogError("No voxel chunk at (" + coord.chunk.toString() + ") for checking triggers.");
		return;
	}

	VoxelChunk &chunk = *chunkPtr;
	const VoxelInt3 voxel = coord.voxel;
	VoxelTriggerDefID triggerDefID;
	if (!chunk.tryGetTriggerDefID(voxel.x, voxel.y, voxel.z, &triggerDefID))
	{
		return;
	}

	const VoxelTriggerDefinition &triggerDef = chunk.triggerDefs[triggerDefID];
	if (triggerDef.hasSoundDef())
	{
		const VoxelTriggerSoundDefinition &soundDef = triggerDef.sound;
		const std::string &soundFilename = soundDef.filename;
		auto &audioManager = game.audioManager;
		audioManager.playSound(soundFilename.c_str());
	}

	if (triggerDef.hasLoreTextDef())
	{
		const VoxelTriggerLoreTextDefinition &textDef = triggerDef.loreText;

		int triggerInstIndex;
		const bool hasBeenTriggered = chunk.tryGetTriggerInstIndex(voxel.x, voxel.y, voxel.z, &triggerInstIndex);
		const bool canDisplay = !textDef.isDisplayedOnce || !hasBeenTriggered;

		if (canDisplay)
		{
			// Ignore the newline at the end.
			const std::string &textDefText = textDef.text;
			const std::string text = textDefText.substr(0, textDefText.size() - 1);
			triggerTextBox.setText(text);
			gameState.setTriggerTextDuration(text);

			// Set the text trigger as activated regardless of whether it's single-shot, just for consistency.
			if (!hasBeenTriggered)
			{
				VoxelTriggerInstance newTriggerInst;
				newTriggerInst.init(voxel.x, voxel.y, voxel.z);
				chunk.triggerInsts.emplace_back(std::move(newTriggerInst));
			}
		}
	}
}

void MapLogic::handleDoorOpen(Game &game, VoxelChunk &voxelChunk, const VoxelInt3 &voxel, double ceilingScale, bool isApplyingDoorKeyToLock, int doorKeyID, bool isWeaponBashing)
{
	VoxelDoorDefID doorDefID;
	if (!voxelChunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
	{
		DebugLogErrorFormat("Expected door def ID to exist at (%s).", voxel.toString().c_str());
		return;
	}

	const VoxelDoorDefinition &doorDef = voxelChunk.doorDefs[doorDefID];
	const VoxelDoorOpenSoundDefinition &openSoundDef = doorDef.openSoundDef;
	const std::string &soundFilename = openSoundDef.soundFilename;
	const CoordDouble3 soundCoord(voxelChunk.position, VoxelUtils::getVoxelCenter(voxel, ceilingScale));
	const WorldDouble3 soundPosition = VoxelUtils::coordToWorldPoint(soundCoord);

	VoxelDoorAnimationInstance newDoorAnimInst;
	newDoorAnimInst.initOpening(voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::DOOR_ANIM_SPEED);
	voxelChunk.doorAnimInsts.emplace_back(std::move(newDoorAnimInst));

	bool isDoorBecomingUnlocked = false;

	int triggerInstIndex;
	const bool hasDoorBeenUnlocked = voxelChunk.tryGetTriggerInstIndex(voxel.x, voxel.y, voxel.z, &triggerInstIndex);
	if (!hasDoorBeenUnlocked)
	{
		VoxelLockDefID lockDefID;
		if (voxelChunk.tryGetLockDefID(voxel.x, voxel.y, voxel.z, &lockDefID))
		{
			isDoorBecomingUnlocked = isApplyingDoorKeyToLock || isWeaponBashing;
		}
	}

	if (isDoorBecomingUnlocked)
	{
		VoxelTriggerInstance newTriggerInst;
		newTriggerInst.init(voxel.x, voxel.y, voxel.z);
		voxelChunk.triggerInsts.emplace_back(std::move(newTriggerInst));
	}

	const bool isDoorKeyUseValid = isApplyingDoorKeyToLock && isDoorBecomingUnlocked;
	if (isDoorKeyUseValid)
	{
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();
		GameWorldUiController::onDoorUnlockedWithKey(game, doorKeyID, soundFilename, soundPosition, exeData);
		game.player.removeFromKeyInventory(doorKeyID);
	}
	else
	{
		AudioManager &audioManager = game.audioManager;
		audioManager.playSound(soundFilename.c_str(), soundPosition);
	}
}

void MapLogic::handleStartDungeonLevelUpVoxelEnter(Game &game)
{
	auto &player = game.player;
	player.setPhysicsVelocity(Double3::Zero);
	player.clearKeyInventory();

	auto &gameState = game.gameState;
	const int provinceID = player.raceID; // @todo: this should be more like a WorldMapDefinition::getProvinceIdForRaceId() that searches provinces
	const int locationID = game.random.next(32); // @todo: this should not assume 32 locations per province

	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationID);

	const ArenaWeatherType weatherType = gameState.getWeatherForLocation(provinceID, locationID);

	const int starCount = SkyUtils::getStarCountFromDensity(game.options.getMisc_StarDensity());
	auto &renderer = game.renderer;

	const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
	Buffer<uint8_t> reservedBlocks = [&cityDef]()
	{
		const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
		DebugAssert(cityReservedBlocks != nullptr);
		Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
		std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.begin());
		return buffer;
	}();

	const std::optional<LocationCityMainQuestTempleOverride> mainQuestTempleOverride =
		[&cityDef]() -> std::optional<LocationCityMainQuestTempleOverride>
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

	MapGenerationCityInfo cityGenInfo;
	cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName), cityDef.type,
		cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade, cityDef.coastal,
		cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
		mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

	const int currentDay = gameState.getDate().getDay();
	const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
	{
		WeatherDefinition weatherDef;
		weatherDef.initFromClassic(weatherType, currentDay, game.random);
		return weatherDef;
	}();

	SkyGenerationExteriorInfo skyGenInfo;
	skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
		cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

	const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceID, locationID);

	MapDefinition mapDefinition;
	if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, game.textureManager))
	{
		DebugLogError("Couldn't init MapDefinition for city \"" + locationDef.getName() + "\".");
		return;
	}

	GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
	{
		// Set music based on weather and time.
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		GameState &gameState = game.gameState;
		const MusicDefinition *musicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		return musicDef;
	};

	gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);
	gameState.queueMusicOnSceneChange(musicFunc);
}

void MapLogic::handleMapTransition(Game &game, const RayCastHit &hit, const TransitionDefinition &transitionDef)
{
	const TransitionType transitionType = transitionDef.type;
	DebugAssert(transitionType != TransitionType::InteriorLevelChange);

	DebugAssert(hit.type == RayCastHitType::Voxel);
	const RayCastVoxelHit &voxelHit = hit.voxelHit;
	const CoordInt3 hitVoxelCoord = voxelHit.voxelCoord;

	auto &gameState = game.gameState;
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
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
			const GameState &gameState = game.gameState;
			const MusicDefinition *musicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing exterior music.");
			}

			return musicDef;
		};

		GameState::SceneChangeMusicFunc jingleMusicDefFunc = [](Game &game)
		{
			// Only play jingle if the exterior is inside the city walls.
			GameState &gameState = game.gameState;
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();

			const MusicDefinition *jingleMusicDef = nullptr;
			if (gameState.getActiveMapDef().getMapType() == MapType::City)
			{
				const LocationDefinition &locationDef = gameState.getLocationDefinition();
				const LocationCityDefinition &locationCityDef = locationDef.getCityDefinition();
				jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Jingle, game.random,
					[&locationCityDef](const MusicDefinition &def)
				{
					DebugAssert(def.type == MusicType::Jingle);
					const JingleMusicDefinition &jingleMusicDef = def.jingle;
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
			const CoordInt3 returnCoord = [&voxelHit, &hitVoxelCoord]()
			{
				const VoxelInt3 delta = [&voxelHit, &hitVoxelCoord]()
				{
					// Assuming this is a wall voxel.
					const VoxelFacing3D facing = voxelHit.facing;

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
						DebugLogError("Invalid EnterInterior returnCoord facing " + std::to_string(static_cast<int>(facing)) + ".");
						return VoxelInt3::Zero;
					}
				}();

				return hitVoxelCoord + delta;
			}();

			const InteriorEntranceTransitionDefinition &interiorEntranceDef = transitionDef.interiorEntrance;
			const MapGenerationInteriorInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;

			MapDefinition mapDefinition;
			if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
			{
				DebugLogError("Couldn't init MapDefinition for interior type " + std::to_string(static_cast<int>(interiorGenInfo.interiorType)) + ".");
				return;
			}

			GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
			{
				// Change to interior music.
				const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
				const MapDefinition &activeMapDef = game.gameState.getActiveMapDef();
				DebugAssert(activeMapDef.getMapType() == MapType::Interior);
				const MapDefinitionInterior &mapDefInterior = activeMapDef.getSubDefinition().interior;
				const ArenaInteriorType interiorType = mapDefInterior.interiorType;
				const InteriorMusicType interiorMusicType = MusicUtils::getInteriorMusicType(interiorType);
				const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
					MusicType::Interior, game.random, [interiorMusicType](const MusicDefinition &def)
				{
					DebugAssert(def.type == MusicType::Interior);
					const InteriorMusicDefinition &interiorMusicDef = def.interior;
					return interiorMusicDef.type == interiorMusicType;
				});

				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing interior music.");
				}

				return musicDef;
			};

			VoxelInt2 playerStartOffset = VoxelInt2::Zero;
			if (interiorGenInfo.type == MapGenerationInteriorType::Dungeon)
			{
				// @temp hack, assume entering a wild dungeon, need to push player south by one due to original map bug.
				playerStartOffset = VoxelUtils::South;
			}

			// Always use clear weather in interiors.
			WeatherDefinition overrideWeather;
			overrideWeather.initClear();

			gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, returnCoord, playerStartOffset, std::nullopt, false, overrideWeather);
			gameState.queueMusicOnSceneChange(musicFunc);
		}
		else if (transitionType == TransitionType::CityGate)
		{
			// City gate transition.
			const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
			const LocationDefinition &locationDef = gameState.getLocationDefinition();
			const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
			const int currentDay = gameState.getDate().getDay();
			const int starCount = SkyUtils::getStarCountFromDensity(game.options.getMisc_StarDensity());

			if (activeMapType == MapType::City)
			{
				// From city to wilderness. Use the gate position to determine where to put the player.

				// The voxel face that was hit determines where to put the player relative to the gate.
				const VoxelInt2 transitionDir = [&voxelHit]()
				{
					// Assuming this is a wall voxel.
					const VoxelFacing3D facing = voxelHit.facing;

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
						DebugLogError("Invalid CityGate transitionDir facing " + std::to_string(static_cast<int>(facing)) + ".");
						return VoxelUtils::North;
					}
				}();

				const auto &exeData = binaryAssetLibrary.getExeData();
				Buffer2D<ArenaWildBlockID> wildBlockIDs = ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

				MapGenerationWildInfo wildGenInfo;
				wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

				SkyGenerationExteriorInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// Calculate wilderness position based on the gate's voxel in the city.
				const CoordInt2 startCoord = [&hitVoxelCoord, &transitionDir]()
				{
					// Origin of the city in the wilderness.
					const ChunkInt2 wildCityChunk(ArenaWildUtils::CITY_ORIGIN_CHUNK_X, ArenaWildUtils::CITY_ORIGIN_CHUNK_Z);

					// Player position bias based on selected gate face.
					const VoxelInt2 offset(transitionDir.x, transitionDir.y);

					return CoordInt2(wildCityChunk + hitVoxelCoord.chunk, VoxelInt2(hitVoxelCoord.voxel.x, hitVoxelCoord.voxel.z) + offset);
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

				const std::optional<LocationCityMainQuestTempleOverride> mainQuestTempleOverride =
					[&cityDef]() -> std::optional<LocationCityMainQuestTempleOverride>
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

				MapGenerationCityInfo cityGenInfo;
				cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
					cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
					cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
					mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY,
					cityDef.cityBlocksPerSide);

				SkyGenerationExteriorInfo skyGenInfo;
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
				const GameState &gameState = game.gameState;
				const MusicDefinition *musicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing exterior music.");
				}

				return musicDef;
			};

			const ArenaCityType cityDefType = cityDef.type;
			const ArenaClimateType cityDefClimateType = cityDef.climateType;
			GameState::SceneChangeMusicFunc jingleMusicFunc = [cityDefType, cityDefClimateType](Game &game)
			{
				// Only play jingle when going wilderness to city.
				GameState &gameState = game.gameState;
				const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
				const MapDefinition &activeMapDef = gameState.getActiveMapDef();
				const MusicDefinition *jingleMusicDef = nullptr;
				if (activeMapDef.getMapType() == MapType::City)
				{
					jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Jingle,
						game.random, [cityDefType, cityDefClimateType](const MusicDefinition &def)
					{
						DebugAssert(def.type == MusicType::Jingle);
						const JingleMusicDefinition &jingleMusicDef = def.jingle;
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

void MapLogic::handleInteriorLevelTransition(Game &game, const CoordInt3 &playerCoord, const CoordInt3 &transitionCoord)
{
	GameState &gameState = game.gameState;

	// Level transitions are always between interiors.
	const MapDefinition &interiorMapDef = gameState.getActiveMapDef();
	DebugAssert(interiorMapDef.getMapType() == MapType::Interior);

	const SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const VoxelChunk *chunkPtr = voxelChunkManager.findChunkAtPosition(transitionCoord.chunk);
	if (chunkPtr == nullptr)
	{
		DebugLogError("No voxel chunk at (" + transitionCoord.chunk.toString() + ") for checking level transition.");
		return;
	}

	const VoxelChunk &chunk = *chunkPtr;
	const VoxelInt3 transitionVoxel = transitionCoord.voxel;
	if (!chunk.isValidVoxel(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z))
	{
		return;
	}

	const VoxelTraitsDefID voxelTraitsDefID = chunk.traitsDefIDs.get(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z);	
	const VoxelTraitsDefinition &voxelTraitsDef = chunk.traitsDefs[voxelTraitsDefID];
	if (voxelTraitsDef.type != ArenaVoxelType::Wall)
	{
		return;
	}

	VoxelTransitionDefID transitionDefID;
	if (!chunk.tryGetTransitionDefID(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z, &transitionDefID))
	{
		return;
	}

	const TransitionDefinition &transitionDef = chunk.transitionDefs[transitionDefID];
	if (transitionDef.type != TransitionType::InteriorLevelChange)
	{
		return;
	}

	// The direction from a level up/down voxel to where the player should end up after going through.
	// It points to the destination voxel adjacent to the level up/down voxel.
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
			DebugLogError("Couldn't determine player direction for transition (" + transitionCoord.voxel.toString() + ") in chunk (" + transitionCoord.chunk.toString() + ").");
			return -Int3::UnitX;
		}
	}();

	Player &player = game.player;
	const VoxelInt2 transitionVoxelXZ = transitionCoord.voxel.getXZ();
	const VoxelInt2 dirToWorldVoxelXZ = dirToWorldVoxel.getXZ();

	// Lambda for opening the world map when the player enters a transition voxel
	// that will "lead to the surface of the dungeon".
	auto switchToWorldMap = [&playerCoord, &game, &gameState, &player]()
	{
		// Move player to center of previous voxel in case they change their mind
		// about fast traveling. Don't change their direction.
		const VoxelInt2 playerVoxelXZ = playerCoord.voxel.getXZ();
		const VoxelDouble2 playerVoxelCenterXZ = VoxelUtils::getVoxelCenter(playerVoxelXZ);
		const VoxelDouble3 playerFeetDestinationPoint(
			playerVoxelCenterXZ.x,
			gameState.getActiveCeilingScale(),
			playerVoxelCenterXZ.y);
		const WorldDouble3 playerFeetDestinationPosition = VoxelUtils::coordToWorldPoint(CoordDouble3(playerCoord.chunk, playerFeetDestinationPoint));
		player.setPhysicsPositionRelativeToFeet(playerFeetDestinationPosition);
		player.setPhysicsVelocity(Double3::Zero);

		game.setPanel<WorldMapPanel>();
	};

	const int activeLevelIndex = gameState.getActiveLevelIndex();
	const int levelCount = interiorMapDef.getLevels().getCount();
	const InteriorLevelChangeTransitionDefinition &levelChangeDef = transitionDef.interiorLevelChange;
	if (levelChangeDef.isLevelUp)
	{
		bool isStartDungeon = false;
		const LocationDefinition &currentLocationDef = gameState.getLocationDefinition();
		if (currentLocationDef.getType() == LocationDefinitionType::MainQuestDungeon)
		{
			const LocationMainQuestDungeonDefinition &locationMainQuestDungeonDef = currentLocationDef.getMainQuestDungeonDefinition();
			if (locationMainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Start)
			{
				isStartDungeon = true;
			}
		}

		bool isCityWildernessDungeon = false;
		if (currentLocationDef.getType() == LocationDefinitionType::City)
		{
			if (gameState.isActiveMapNested())
			{
				// Can assume that only wild dungeons have *LEVELUP voxels on top floor in a city location.
				isCityWildernessDungeon = true;
			}
		}

		if (activeLevelIndex > 0)
		{
			// Decrement the world's level index and activate the new level.
			gameState.queueLevelIndexChange(activeLevelIndex - 1, transitionVoxelXZ, dirToWorldVoxelXZ);
		}
		else if (isStartDungeon)
		{
			MapLogic::handleStartDungeonLevelUpVoxelEnter(game);
		}
		else if (isCityWildernessDungeon)
		{
			GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
			{
				GameState &gameState = game.gameState;
				const MusicDefinition *musicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing exterior music.");
				}

				return musicDef;
			};

			gameState.queueMapDefPop();
			gameState.queueMusicOnSceneChange(musicFunc);
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
			gameState.queueLevelIndexChange(activeLevelIndex + 1, transitionVoxelXZ, dirToWorldVoxelXZ);
		}
		else
		{
			switchToWorldMap();
		}
	}
}
