#include <algorithm>
#include <sstream>

#include "GameWorldUiState.h"
#include "MainQuestSplashUiState.h"
#include "WorldMapUiMVC.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"
#include "../Math/Random.h"
#include "../Sky/SkyUtils.h"
#include "../Time/ArenaDateUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string WorldMapUiModel::getProvinceNameOffsetFilename()
{
	return "OUTPROV.CIF";
}

const WorldMapMask &WorldMapUiModel::getMask(const Game &game, int maskID)
{
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &worldMapMasks = binaryAssetLibrary.getWorldMapMasks();

	DebugAssertIndex(worldMapMasks, maskID);
	return worldMapMasks[maskID];
}

std::optional<int> WorldMapUiModel::getMaskID(Game &game, const Int2 &mousePosition, bool ignoreCenterProvince,
	bool ignoreExitButton)
{
	const Int2 classicPosition = game.window.nativeToOriginal(mousePosition);
	const auto &worldMapMasks = BinaryAssetLibrary::getInstance().getWorldMapMasks();
	const int maskCount = static_cast<int>(worldMapMasks.size());
	for (int maskID = 0; maskID < maskCount; maskID++)
	{
		constexpr int centerProvinceID = ArenaLocationUtils::CENTER_PROVINCE_ID;
		constexpr int exitButtonID = WorldMapUiModel::EXIT_BUTTON_MASK_ID;
		if ((ignoreCenterProvince && (maskID == centerProvinceID)) ||
			(ignoreExitButton && (maskID == exitButtonID)))
		{
			continue;
		}

		DebugAssertIndex(worldMapMasks, maskID);
		const WorldMapMask &mapMask = worldMapMasks[maskID];
		const Rect &maskRect = mapMask.getRect();

		if (maskRect.contains(classicPosition))
		{
			// See if the pixel is set in the bitmask.
			const bool success = mapMask.get(classicPosition.x, classicPosition.y);

			if (success)
			{
				return maskID;
			}
		}
	}

	// No province/button found at the given pixel.
	return std::nullopt;
}

void FastTravelUiModel::tickTravelTime(Game &game, int travelDays)
{
	DebugAssert(travelDays >= 0);

	auto &gameState = game.gameState;
	Random &random = game.random;

	Date &date = gameState.getDate();
	for (int i = 0; i < travelDays; i++)
	{
		date.incrementDay();
	}

	Clock &clock = gameState.getClock();
	const int randomHours = random.next(23);
	for (int i = 0; i < randomHours; i++)
	{
		clock.incrementHour();

		if (clock.hours == 0)
		{
			date.incrementDay();
		}
	}
}

std::string FastTravelUiModel::getCityArrivalMessage(Game &game, int targetProvinceID,
	int targetLocationID, int travelDays)
{
	auto &gameState = game.gameState;
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();

	// @todo: change these to 'index' so it's clear they don't rely on original game's 0-32, etc..
	const int provinceID = targetProvinceID;
	const int localCityID = targetLocationID;

	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
	const LocationDefinition &locationDef = provinceDef.getLocationDef(localCityID);

	const std::string locationString = [travelDays, &gameState, &exeData, provinceID,
		localCityID, &provinceDef, &locationDef]()
	{
		if (provinceID != ArenaLocationUtils::CENTER_PROVINCE_ID)
		{
			// The <city type> of <city name> in <province> Province.
			// Replace first %s with location type.
			const std::string &locationTypeName = [&exeData, localCityID]()
			{
				int locationTypeIndex = -1;
				if (localCityID < 8)
				{
					// City.
					locationTypeIndex = 0;
				}
				else if (localCityID < 16)
				{
					// Town.
					locationTypeIndex = 1;
				}
				else
				{
					// Village.
					locationTypeIndex = 2;
				}

				return exeData.locations.locationTypes[locationTypeIndex];
			}();

			std::string text = exeData.travel.locationFormatTexts[2];

			// Replace first %s with location type name.
			size_t index = text.find("%s");
			text.replace(index, 2, locationTypeName);

			// Replace second %s with location name.
			index = text.find("%s", index);
			text.replace(index, 2, locationDef.getName());

			// Replace third %s with province name.
			index = text.find("%s", index);
			text.replace(index, 2, provinceDef.getName());

			return exeData.travel.arrivalPopUpLocation + text;
		}
		else
		{
			// Center province displays only the city name.
			return exeData.travel.arrivalPopUpLocation + exeData.travel.arrivalCenterProvinceLocation;
		}
	}();

	const std::string dateString = exeData.travel.arrivalPopUpDate +
		ArenaDateUtils::makeDateString(gameState.getDate(), exeData);

	const std::string daysString = [travelDays, &exeData]()
	{
		std::string text = exeData.travel.arrivalPopUpDays;

		// Replace %d with travel days.
		size_t index = text.find("%d");
		text.replace(index, 2, std::to_string(travelDays));

		return text;
	}();

	const std::string locationDescriptionString = [&game, &gameState, &binaryAssetLibrary,
		&exeData, provinceID, localCityID, &locationDef]()
	{
		const ArenaLocationType locationType = ArenaLocationUtils::getCityType(localCityID);

		// Get the description for the local location. If it's a town or village, choose
		// one of the three substrings randomly. Otherwise, get the city description text
		// directly.
		const std::string description = [&game, &binaryAssetLibrary, &exeData, provinceID, localCityID, &locationDef, locationType]()
		{
			// City descriptions start at #0600. The three town descriptions are at #1422,
			// and the three village descriptions are at #1423.
			const int templateDatEntryKey = [provinceID, localCityID, locationType]()
			{
				if (locationType == ArenaLocationType::CityState)
				{
					return 600 + localCityID + (8 * provinceID);
				}
				else if (locationType == ArenaLocationType::Town)
				{
					return 1422;
				}
				else if (locationType == ArenaLocationType::Village)
				{
					return 1423;
				}
				else
				{
					DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(locationType)));
				}
			}();

			const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
			const ArenaTemplateDat &templateDat = textAssetLibrary.templateDat;
			const ArenaTemplateDatEntry &entry = templateDat.getEntry(templateDatEntryKey);
			const Span<const std::string> templateDatTexts = entry.values;

			if (locationType == ArenaLocationType::CityState)
			{
				return templateDatTexts[0];
			}
			else
			{
				ArenaRandom &random = game.arenaRandom;
				std::string description = [&random, &templateDatTexts]()
				{
					const int templateDatTextIndex = random.next() % templateDatTexts.getCount();
					return templateDatTexts[templateDatTextIndex];
				}();

				// Replace %cn with city name.
				size_t index = description.find("%cn");
				description.replace(index, 3, locationDef.getName());

				const auto &cityData = binaryAssetLibrary.getCityDataFile();
				const auto &province = cityData.getProvinceData(provinceID);
				const uint32_t rulerSeed = [localCityID, &cityData, &province]()
				{
					const auto &location = province.getLocationData(localCityID);
					const Int2 localPoint(location.x, location.y);
					return ArenaLocationUtils::getRulerSeed(localPoint, province.getGlobalRect());
				}();

				const bool isMale = ArenaLocationUtils::isRulerMale(localCityID, province);

				// Replace %t with ruler title (if it exists).
				random.srand(rulerSeed);
				index = description.find("%t");
				if (index != std::string::npos)
				{
					const std::string &rulerTitle = binaryAssetLibrary.getRulerTitle(provinceID, locationType, isMale, random);
					description.replace(index, 2, rulerTitle);
				}

				// Replace %rf with ruler first name (if it exists). Make sure to reset the random seed.
				random.srand(rulerSeed);
				index = description.find("%rf");
				if (index != std::string::npos)
				{
					const std::string rulerFirstName = [provinceID, &random, isMale]()
					{
						const auto &textAssetLibrary = TextAssetLibrary::getInstance();
						const std::string fullName = textAssetLibrary.generateNpcName(provinceID, isMale, random);
						const Buffer<std::string> tokens = String::split(fullName, ' ');
						return tokens[0];
					}();

					description.replace(index, 3, rulerFirstName);
				}

				return description;
			}
		}();

		return description;
	}();

	std::string fullText = locationString + dateString + daysString + locationDescriptionString;

	// Replace all line breaks with spaces and compress spaces into one.
	std::string trimmedText = [&fullText]()
	{
		std::string str;
		char prev = -1;
		for (char c : fullText)
		{
			if (c == '\r')
			{
				c = ' ';
			}

			if (prev != ' ' || c != ' ')
			{
				str += c;
			}

			prev = c;
		}

		return str;
	}();

	// Re-distribute newlines.
	fullText = String::distributeNewlines(trimmedText, 50);

	return fullText;
}

Int2 WorldMapUiView::getProvinceNameOffset(int provinceID, TextureManager &textureManager)
{
	const std::string provinceNameOffsetFilename = WorldMapUiModel::getProvinceNameOffsetFilename();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(provinceNameOffsetFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + provinceNameOffsetFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	return textureFileMetadata.getOffset(provinceID);
}

TextureAsset WorldMapUiView::getPaletteTextureAsset()
{
	return TextureAsset(ArenaTextureName::WorldMap);
}

std::string WorldMapUiView::getProvinceNamesFilename()
{
	return ArenaTextureName::ProvinceNames;
}

UiTextureID WorldMapUiView::allocHighlightedTextTexture(int provinceID, TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = WorldMapUiView::getPaletteTextureAsset();

	const std::string provinceNamesFilename = WorldMapUiView::getProvinceNamesFilename();
	const TextureAsset textureAsset(provinceNamesFilename, provinceID);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for highlighted text for province " + std::to_string(provinceID) + ".");
	}

	return textureID;
}

Int2 FastTravelUiView::getAnimationTextureCenter()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
}

std::string FastTravelUiView::getAnimationFilename()
{
	return ArenaTextureName::FastTravel;
}

TextureAsset FastTravelUiView::getPaletteTextureAsset()
{
	return TextureAsset(ArenaTextureName::WorldMap);
}

void FastTravelUiController::onAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays)
{
	// Clear selected map location.
	auto &gameState = game.gameState;
	gameState.setTravelData(std::nullopt);

	// Handle fast travel behavior and decide which UI to switch to.
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

	// Update game clock.
	// @todo: maybe move this to a WorldMapLogicController namespace
	FastTravelUiModel::tickTravelTime(game, travelDays);

	// Update weathers.
	gameState.updateWeatherList(game.arenaRandom, exeData);

	// Clear just the lore text (action text and effect text are unchanged).
	GameWorldUI::resetTriggerTextDuration();

	// Clear keys inventory in case we're leaving a main quest dungeon.
	Player &player = game.player;
	player.clearKeyInventory();

	const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
	const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);

	TextureManager &textureManager = game.textureManager;

	// Decide how to load the location.
	if (travelLocationDef.getType() == LocationDefinitionType::City)
	{
		// Get weather type from game state.
		const LocationCityDefinition &cityDef = travelLocationDef.getCityDefinition();
		const ArenaWeatherType weatherType = [&game, &gameState, &binaryAssetLibrary,
			&travelProvinceDef, &travelLocationDef, &cityDef]()
		{
			const Int2 localPoint(travelLocationDef.getScreenX(), travelLocationDef.getScreenY());
			const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(localPoint, travelProvinceDef.getGlobalRect());

			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const int globalQuarter = ArenaLocationUtils::getGlobalQuarter(globalPoint, cityData);

			Span<const ArenaWeatherType> worldMapWeathers = gameState.getWorldMapWeathers();
			DebugAssertIndex(worldMapWeathers, globalQuarter);
			return ArenaWeatherUtils::getFilteredWeatherType(worldMapWeathers[globalQuarter], cityDef.climateType);
		}();

		const int starCount = SkyUtils::getStarCountFromDensity(game.options.getMisc_StarDensity());

		// Get city generation values.
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
			cityDef.citySeed, cityDef.rulerSeed, travelProvinceDef.getRaceID(), cityDef.premade, cityDef.coastal,
			cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks), mainQuestTempleOverride,
			cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		const int currentDay = gameState.getDate().getDay();
		const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
		{
			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(weatherType, currentDay, game.random);
			return weatherDef;
		}();

		SkyGenerationExteriorInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, travelProvinceDef.hasAnimatedDistantLand());

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, textureManager))
		{
			DebugCrash("Couldn't init MapDefinition for city \"" + travelLocationDef.getName() + "\".");
			return;
		}

		GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
		{
			// Choose time-based music and enter the game world.
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
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
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
			const MusicDefinition *jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicType::Jingle, game.random,
				[cityDefType, cityDefClimateType](const MusicDefinition &def)
			{
				DebugAssert(def.type == MusicType::Jingle);
				const JingleMusicDefinition &jingleMusicDef = def.jingle;
				return (jingleMusicDef.cityType == cityDefType) && (jingleMusicDef.climateType == cityDefClimateType);
			});

			if (jingleMusicDef == nullptr)
			{
				DebugLogWarning("Missing jingle music.");
			}

			return jingleMusicDef;
		};

		// Load the destination city.
		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);
		gameState.queueMusicOnSceneChange(musicFunc, jingleMusicFunc);

		GameWorldUiInitInfo &gameWorldInitInfo = GameWorldUI::state.initInfo;
		gameWorldInitInfo.init(FastTravelUiModel::getCityArrivalMessage(game, targetProvinceID, targetLocationID, travelDays));
		game.setNextContext(GameWorldUI::ContextName);
	}
	else if (travelLocationDef.getType() == LocationDefinitionType::Dungeon)
	{
		// Named dungeon.
		constexpr bool isArtifactDungeon = false;
		const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
		const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);
		const LocationDungeonDefinition &dungeonDef = travelLocationDef.getDungeonDefinition();

		MapGenerationInteriorInfo interiorGenInfo;
		interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

		const VoxelInt2 playerStartOffset(
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
		{
			DebugCrash("Couldn't init MapDefinition for named dungeon \"" + travelLocationDef.getName() + "\".");
			return;
		}

		GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
		{
			const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing dungeon music.");
			}

			return musicDef;
		};

		// Always use clear weather in interiors.
		WeatherDefinition overrideWeather;
		overrideWeather.initClear();

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, playerStartOffset, worldMapLocationIDs, true, overrideWeather);
		gameState.queueMusicOnSceneChange(musicFunc);

		game.setNextContext(GameWorldUI::ContextName);
	}
	else if (travelLocationDef.getType() == LocationDefinitionType::MainQuestDungeon)
	{
		// Main quest dungeon. The staff dungeons have a splash image before going to the game world UI.
		const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = travelLocationDef.getMainQuestDungeonDefinition();

		constexpr std::optional<bool> rulerIsMale; // Not needed.
		const std::string interiorDisplayName; // Unused.

		MapGenerationInteriorInfo interiorGenInfo;
		interiorGenInfo.initPrefab(mainQuestDungeonDef.mapFilename, ArenaInteriorType::Dungeon, rulerIsMale, interiorDisplayName);

		const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest dungeon.
		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
		{
			DebugLogError("Couldn't init MapDefinition for main quest interior \"" + travelLocationDef.getName() + "\".");
			return;
		}

		// Always use clear weather in interiors.
		WeatherDefinition overrideWeather;
		overrideWeather.initClear();

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);

		if (mainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Staff)
		{
			// Go to staff dungeon splash image first.
			MainQuestSplashUI::state.provinceID = targetProvinceID;
			game.setNextContext(MainQuestSplashUI::ContextName);
		}
		else
		{
			GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
			{
				const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing dungeon music.");
				}

				return musicDef;
			};

			gameState.queueMusicOnSceneChange(musicFunc);
			game.setNextContext(GameWorldUI::ContextName);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(travelLocationDef.getType())));
	}
}
