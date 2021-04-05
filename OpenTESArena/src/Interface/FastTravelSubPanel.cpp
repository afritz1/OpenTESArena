#include <algorithm>
#include <sstream>

#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "TextSubPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/MIFFile.h"
#include "../Audio/MusicUtils.h"
#include "../Game/ArenaDateUtils.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"
#include "../World/SkyUtils.h"
#include "../World/WeatherUtils.h"
#include "../WorldMap/LocationDefinition.h"
#include "../WorldMap/LocationType.h"
#include "../WorldMap/LocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

FastTravelSubPanel::FastTravelSubPanel(Game &game, const ProvinceMapPanel::TravelData &travelData)
	: Panel(game), travelData(travelData)
{
	this->currentSeconds = 0.0;
	this->totalSeconds = 0.0;

	// Determine how long the animation should run until switching to the game world.
	this->targetSeconds = std::max(1.0, static_cast<double>(travelData.travelDays) / 25.0);

	this->frameIndex = 0;
}

const std::string &FastTravelSubPanel::getBackgroundFilename() const
{
	return ArenaTextureName::WorldMap;
}

TextureBuilderIdGroup FastTravelSubPanel::getAnimationTextureIDs() const
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();

	const std::string &textureFilename = ArenaTextureName::FastTravel;
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(textureFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + textureFilename + "\".");
	}

	return *textureBuilderIDs;
}

std::unique_ptr<Panel> FastTravelSubPanel::makeCityArrivalPopUp() const
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	const Int2 center = GameWorldPanel::getInterfaceCenter(modernInterface, textureManager) - Int2(0, 1);

	const std::string text = [this, &game]()
	{
		auto &gameState = game.getGameState();
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		const auto &exeData = binaryAssetLibrary.getExeData();

		// @todo: change these to 'index' so it's clear they don't rely on original game's 0-32, etc..
		const int provinceID = this->travelData.provinceID;
		const int localCityID = this->travelData.locationID;

		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
		const LocationDefinition &locationDef = provinceDef.getLocationDef(localCityID);

		const std::string locationString = [this, &gameState, &exeData, provinceID, localCityID,
			&provinceDef, &locationDef]()
		{
			if (provinceID != LocationUtils::CENTER_PROVINCE_ID)
			{
				// The <city type> of <city name> in <province> Province.
				// Replace first %s with location type.
				const std::string &locationTypeName = [&exeData, localCityID]()
				{
					if (localCityID < 8)
					{
						// City.
						return exeData.locations.locationTypes.front();
					}
					else if (localCityID < 16)
					{
						// Town.
						return exeData.locations.locationTypes.at(1);
					}
					else
					{
						// Village.
						return exeData.locations.locationTypes.at(2);
					}
				}();

				std::string text = exeData.travel.locationFormatTexts.at(2);

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
				return exeData.travel.arrivalPopUpLocation +
					exeData.travel.arrivalCenterProvinceLocation;
			}
		}();

		const std::string dateString = [&gameState, &exeData]()
		{
			return exeData.travel.arrivalPopUpDate + ArenaDateUtils::makeDateString(gameState.getDate(), exeData);
		}();

		const std::string daysString = [this, &exeData]()
		{
			std::string text = exeData.travel.arrivalPopUpDays;

			// Replace %d with travel days.
			size_t index = text.find("%d");
			text.replace(index, 2, std::to_string(this->travelData.travelDays));

			return text;
		}();

		const auto &textAssetLibrary = game.getTextAssetLibrary();
		const std::string locationDescriptionString = [&game, &gameState, &binaryAssetLibrary,
			&textAssetLibrary, &exeData, provinceID, localCityID, &locationDef]()
		{
			const LocationType locationType = LocationUtils::getCityType(localCityID);

			// Get the description for the local location. If it's a town or village, choose
			// one of the three substrings randomly. Otherwise, get the city description text
			// directly.
			const std::string description = [&gameState, &binaryAssetLibrary, &textAssetLibrary,
				&exeData, provinceID, localCityID, &locationDef, locationType]()
			{
				// City descriptions start at #0600. The three town descriptions are at #1422,
				// and the three village descriptions are at #1423.
				const std::vector<std::string> &templateDatTexts = [&binaryAssetLibrary,
					&textAssetLibrary, provinceID, localCityID, locationType]()
				{
					// Get the key that maps into TEMPLATE.DAT.
					const int key = [provinceID, localCityID, locationType]()
					{
						if (locationType == LocationType::CityState)
						{
							return 600 + localCityID + (8 * provinceID);
						}
						else if (locationType == LocationType::Town)
						{
							return 1422;
						}
						else if (locationType == LocationType::Village)
						{
							return 1423;
						}
						else
						{
							DebugUnhandledReturnMsg(int,
								std::to_string(static_cast<int>(locationType)));
						}
					}();

					const auto &templateDat = textAssetLibrary.getTemplateDat();
					const auto &entry = templateDat.getEntry(key);
					return entry.values;
				}();

				if (locationType == LocationType::CityState)
				{
					return templateDatTexts.front();
				}
				else
				{
					ArenaRandom &random = gameState.getRandom();
					std::string description = [&random, &templateDatTexts]()
					{
						return templateDatTexts.at(random.next() % templateDatTexts.size());
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
						return LocationUtils::getRulerSeed(localPoint, province.getGlobalRect());
					}();

					const bool isMale = LocationUtils::isRulerMale(localCityID, province);

					// Replace %t with ruler title (if it exists).
					random.srand(rulerSeed);
					index = description.find("%t");
					if (index != std::string::npos)
					{
						const std::string &rulerTitle = binaryAssetLibrary.getRulerTitle(
							provinceID, locationType, isMale, random);
						description.replace(index, 2, rulerTitle);
					}

					// Replace %rf with ruler first name (if it exists). Make sure to reset
					// the random seed.
					random.srand(rulerSeed);
					index = description.find("%rf");
					if (index != std::string::npos)
					{
						const std::string rulerFirstName = [&textAssetLibrary, provinceID,
							&random, isMale]()
						{
							const std::string fullName = textAssetLibrary.generateNpcName(
								provinceID, isMale, random);
							const std::vector<std::string> tokens = String::split(fullName, ' ');
							return tokens.front();
						}();

						description.replace(index, 3, rulerFirstName);
					}

					return description;
				}
			}();

			return description;
		}();

		std::string fullText = locationString + dateString +
			daysString + locationDescriptionString;

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
	}();

	const int lineSpacing = 1;
	const Color color(251, 239, 77);

	const RichTextString richText(
		text,
		FontName::Arena,
		color,
		TextAlignment::Center,
		lineSpacing,
		game.getFontLibrary());

	Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark,
		richText.getDimensions().x + 10, richText.getDimensions().y + 12,
		textureManager, renderer);

	const Int2 textureCenter(center.x, center.y + 1);

	auto function = [](Game &game)
	{
		game.popSubPanel();
	};

	return std::make_unique<TextSubPanel>(game, center, richText, function,
		std::move(texture), textureCenter);
}

std::optional<Panel::CursorData> FastTravelSubPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void FastTravelSubPanel::tickTravelTime(Random &random) const
{
	auto &game = this->getGame();
	auto &gameState = game.getGameState();

	// Tick the game date by the number of travel days.
	auto &date = gameState.getDate();
	for (int i = 0; i < this->travelData.travelDays; i++)
	{
		date.incrementDay();
	}

	// Add between 0 and 22 random hours to the clock time.
	auto &clock = gameState.getClock();
	const int randomHours = random.next(23);
	for (int i = 0; i < randomHours; i++)
	{
		clock.incrementHour();

		// Increment day if the clock loops around.
		if (clock.getHours24() == 0)
		{
			date.incrementDay();
		}
	}
}

void FastTravelSubPanel::switchToNextPanel()
{
	// Handle fast travel behavior and decide which panel to switch to.
	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

	// Update game clock.
	this->tickTravelTime(game.getRandom());

	// Update weathers.
	gameState.updateWeatherList(exeData);

	// Clear the lore text (action text and effect text are unchanged).
	gameState.resetTriggerText();

	// Clear any on-voxel-enter event to avoid things like fast travelling out of the
	// starting dungeon then being teleported to a random city when going through any
	// subsequent LEVELUP voxel.
	gameState.getOnLevelUpVoxelEnter() = std::function<void(Game&)>();

	// Pop this sub-panel on the next game loop. The game loop pops old sub-panels before
	// pushing new ones, so call order doesn't matter.
	game.popSubPanel();

	const auto &travelProvinceDef = worldMapDef.getProvinceDef(this->travelData.provinceID);
	const auto &travelLocationDef = travelProvinceDef.getLocationDef(this->travelData.locationID);

	// Decide how to load the location.
	if (travelLocationDef.getType() == LocationDefinition::Type::City)
	{
		// Get weather type from game state.
		const LocationDefinition::CityDefinition &cityDef = travelLocationDef.getCityDefinition();
		const WeatherType weatherType = [this, &game, &gameState, &binaryAssetLibrary,
			&travelProvinceDef, &travelLocationDef, &cityDef]()
		{
			const Int2 localPoint(travelLocationDef.getScreenX(), travelLocationDef.getScreenY());
			const Int2 globalPoint = LocationUtils::getGlobalPoint(
				localPoint, travelProvinceDef.getGlobalRect());

			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const int globalQuarter = LocationUtils::getGlobalQuarter(globalPoint, cityData);

			const auto &weathersArray = gameState.getWeathersArray();
			DebugAssertIndex(weathersArray, globalQuarter);
			return WeatherUtils::getFilteredWeatherType(weathersArray[globalQuarter], cityDef.climateType);
		}();

		const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());

		// Get city generation values.
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
			DebugAssert(cityReservedBlocks != nullptr);
			Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
			std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
			return buffer;
		}();

		const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() ->std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
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
		cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName), cityDef.type,
			cityDef.citySeed, cityDef.rulerSeed, travelProvinceDef.getRaceID(), cityDef.premade, cityDef.coastal,
			cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks), mainQuestTempleOverride,
			cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		const int currentDay = gameState.getDate().getDay();

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, weatherType, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, travelProvinceDef.hasAnimatedDistantLand());

		// Load the destination city.
		const std::optional<WeatherType> overrideWeather = weatherType;
		const GameState::WorldMapLocationIDs worldMapLocationIDs(this->travelData.provinceID, this->travelData.locationID);
		if (!gameState.trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			game.getTextAssetLibrary(), game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load city \"" + travelLocationDef.getName() + "\".");
		}

		// Choose time-based music and enter the game world.
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = [&game, &gameState, weatherType, &musicLibrary]()
		{
			if (!gameState.nightMusicIsActive())
			{
				return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [weatherType](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.type == weatherType;
				});
			}
			else
			{
				return musicLibrary.getRandomMusicDefinition(
					MusicDefinition::Type::Night, game.getRandom());
			}
		}();

		const MusicDefinition *jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(
			MusicDefinition::Type::Jingle, game.getRandom(), [&cityDef](const MusicDefinition &def)
		{
			DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
			const auto &jingleMusicDef = def.getJingleMusicDefinition();
			return (jingleMusicDef.cityType == cityDef.type) &&
				(jingleMusicDef.climateType == cityDef.climateType);
		});

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		if (jingleMusicDef == nullptr)
		{
			DebugLogWarning("Missing jingle music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef, jingleMusicDef);

		game.setPanel<GameWorldPanel>(game);

		// Push a text sub-panel for the city arrival pop-up.
		std::unique_ptr<Panel> arrivalPopUp = this->makeCityArrivalPopUp();
		game.pushSubPanel(std::move(arrivalPopUp));
	}
	else if (travelLocationDef.getType() == LocationDefinition::Type::Dungeon)
	{
		// Random named dungeon.
		constexpr bool isArtifactDungeon = false;
		const auto &travelProvinceDef = worldMapDef.getProvinceDef(this->travelData.provinceID);
		const auto &travelLocationDef = travelProvinceDef.getLocationDef(this->travelData.locationID);
		const LocationDefinition::DungeonDefinition &dungeonDef = travelLocationDef.getDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

		const std::optional<VoxelInt2> playerStartOffset = VoxelInt2(
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

		const GameState::WorldMapLocationIDs worldMapLocationIDs(this->travelData.provinceID, this->travelData.locationID);
		if (!gameState.trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load named dungeon \"" + travelLocationDef.getName() + "\".");
		}

		// Choose random dungeon music and enter game world.
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
			MusicDefinition::Type::Interior, game.getRandom(), [](const MusicDefinition &def)
		{
			DebugAssert(def.getType() == MusicDefinition::Type::Interior);
			const auto &interiorMusicDef = def.getInteriorMusicDefinition();
			return interiorMusicDef.type == MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
		});

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing dungeon music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef);

		game.setPanel<GameWorldPanel>(game);
	}
	else if (travelLocationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
	{
		// Main quest dungeon. The staff dungeons have a splash image before going to the game world panel.
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
			travelLocationDef.getMainQuestDungeonDefinition();

		constexpr std::optional<bool> rulerIsMale; // Not needed.

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initPrefab(std::string(mainQuestDungeonDef.mapFilename),
			ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest dungeon.

		const GameState::WorldMapLocationIDs worldMapLocationIDs(this->travelData.provinceID, this->travelData.locationID);
		if (!gameState.trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load main quest interior \"" + travelLocationDef.getName() + "\".");
		}

		if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Staff)
		{
			// Go to staff dungeon splash image first.
			game.setPanel<MainQuestSplashPanel>(game, this->travelData.provinceID);
		}
		else
		{
			// Choose random dungeon music and enter game world.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Interior, game.getRandom(), [](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Interior);
				const auto &interiorMusicDef = def.getInteriorMusicDefinition();
				return interiorMusicDef.type == MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing dungeon music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);

			game.setPanel<GameWorldPanel>(game);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(travelLocationDef.getType())));
	}
}

void FastTravelSubPanel::tick(double dt)
{
	// Update horse animation.
	this->currentSeconds += dt;
	while (this->currentSeconds >= FastTravelSubPanel::FRAME_TIME)
	{
		this->currentSeconds -= FastTravelSubPanel::FRAME_TIME;
		this->frameIndex++;

		if (this->frameIndex == this->getAnimationTextureIDs().getCount())
		{
			this->frameIndex = 0;
		}
	}

	// Update total seconds and see if the animation should be done.
	this->totalSeconds += dt;
	if (this->totalSeconds >= this->targetSeconds)
	{
		this->switchToNextPanel();
	}
}

void FastTravelSubPanel::render(Renderer &renderer)
{
	// Draw horse animation.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &paletteFilename = this->getBackgroundFilename();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return;
	}

	const TextureBuilderIdGroup textureBuilderIDs = this->getAnimationTextureIDs();
	const TextureBuilderID textureBuilderID = textureBuilderIDs.getID(static_cast<int>(this->frameIndex));
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);

	const int x = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureBuilder.getWidth() / 2);
	const int y = (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureBuilder.getHeight() / 2);
	renderer.drawOriginal(textureBuilderID, *paletteID, x, y, textureManager);
}
