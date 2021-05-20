#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "GameWorldPanel.h"
#include "ImageSequencePanel.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "MainMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Math/Random.h"
#include "../Math/RandomUtils.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"
#include "../World/ArenaWeatherUtils.h"
#include "../World/MapType.h"
#include "../World/SkyUtils.h"
#include "../WorldMap/LocationType.h"
#include "../WorldMap/LocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	const int MaxTestTypes = 5;
	const int TestType_MainQuest = 0;
	const int TestType_Interior = 1;
	const int TestType_City = 2;
	const int TestType_Wilderness = 3;
	const int TestType_Dungeon = 4;

	// Main quest locations. There are eight map dungeons and eight staff dungeons.
	// The special cases are the start dungeon and the final dungeon.
	const int MainQuestLocationCount = 18;

	// Small hack for main menu testing.
	enum class SpecialCaseType { None, StartDungeon };

	std::string GetTestTypeName(int type)
	{
		if (type == TestType_MainQuest)
		{
			return "Main Quest";
		}
		else if (type == TestType_Interior)
		{
			return "Interior";
		}
		else if (type == TestType_City)
		{
			return "City";
		}
		else if (type == TestType_Wilderness)
		{
			return "Wilderness";
		}
		else
		{
			return "Dungeon";
		}
	}

	void GetMainQuestLocationFromIndex(int testIndex, const ExeData &exeData,
		int *outLocationID, int *outProvinceID, SpecialCaseType *outSpecialCaseType)
	{
		if (testIndex == 0)
		{
			*outLocationID = -1;
			*outProvinceID = LocationUtils::CENTER_PROVINCE_ID;
			*outSpecialCaseType = SpecialCaseType::StartDungeon;
		}
		else if (testIndex == (MainQuestLocationCount - 1))
		{
			*outLocationID = 0;
			*outProvinceID = LocationUtils::CENTER_PROVINCE_ID;
			*outSpecialCaseType = SpecialCaseType::None;
		}
		else
		{
			// Generate the location from the executable data.
			const auto &staffProvinces = exeData.locations.staffProvinces;
			const int staffProvincesIndex = (testIndex - 1) / 2;
			DebugAssertIndex(staffProvinces, staffProvincesIndex);
			*outProvinceID = staffProvinces[staffProvincesIndex];
			*outLocationID = LocationUtils::dungeonToLocationID(testIndex % 2);
			*outSpecialCaseType = SpecialCaseType::None;
		}
	}

	std::vector<int> MakeShuffledLocationIndices(const ProvinceDefinition &provinceDef)
	{
		std::vector<int> indices(provinceDef.getLocationCount());
		std::iota(indices.begin(), indices.end(), 0);
		RandomUtils::shuffle(indices.data(), static_cast<int>(indices.size()));
		return indices;
	}

	std::optional<int> GetRandomCityLocationDefIndexIfType(const ProvinceDefinition &provinceDef,
		ArenaTypes::CityType cityType)
	{
		// Iterate over locations in the province in a random order.
		const std::vector<int> randomLocationIndices = MakeShuffledLocationIndices(provinceDef);

		for (const int locationIndex : randomLocationIndices)
		{
			const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
			if (curLocationDef.getType() == LocationDefinition::Type::City)
			{
				const auto &curCityDef = curLocationDef.getCityDefinition();
				if (curCityDef.type == cityType)
				{
					return locationIndex;
				}
			}
		}

		return std::nullopt;
	}

	int GetRandomCityLocationIndex(const ProvinceDefinition &provinceDef)
	{
		// Iterate over locations in the province in a random order.
		const std::vector<int> randomLocationIndices = MakeShuffledLocationIndices(provinceDef);

		for (const int locationIndex : randomLocationIndices)
		{
			const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
			if (curLocationDef.getType() == LocationDefinition::Type::City)
			{
				return locationIndex;
			}
		}

		return -1;
	}

	std::optional<int> GetRandomDungeonLocationDefIndex(const ProvinceDefinition &provinceDef)
	{
		// Iterate over locations in the province in a random order.
		const std::vector<int> randomLocationIndices = MakeShuffledLocationIndices(provinceDef);

		for (const int locationIndex : randomLocationIndices)
		{
			const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
			if (curLocationDef.getType() == LocationDefinition::Type::Dungeon)
			{
				return locationIndex;
			}
		}

		return std::nullopt;
	}

	// Prefixes for some .MIF files, with an inclusive min/max range of ID suffixes.
	// These also need ".MIF" appended at the end.
	const std::vector<std::tuple<std::string, std::pair<int, int>, ArenaTypes::InteriorType>> InteriorLocations =
	{
		{ "BS", { 1, 8 }, ArenaTypes::InteriorType::House },
		{ "EQUIP", { 1, 8 }, ArenaTypes::InteriorType::Equipment },
		{ "MAGE", { 1, 8 }, ArenaTypes::InteriorType::MagesGuild },
		{ "NOBLE", { 1, 8 }, ArenaTypes::InteriorType::Noble },
		{ "PALACE", { 1, 5 }, ArenaTypes::InteriorType::Palace },
		{ "TAVERN", { 1, 8 }, ArenaTypes::InteriorType::Tavern },
		{ "TEMPLE", { 1, 8 }, ArenaTypes::InteriorType::Temple },
		{ "TOWER", { 1, 8 }, ArenaTypes::InteriorType::Tower },
		{ "TOWNPAL", { 1, 3 }, ArenaTypes::InteriorType::Palace },
		{ "VILPAL", { 1, 3 }, ArenaTypes::InteriorType::Palace },
		{ "WCRYPT", { 1, 8 }, ArenaTypes::InteriorType::Crypt }
	};

	const std::string ImperialMIF = "IMPERIAL.MIF";
	const std::string RandomCity = "Random City";
	const std::string RandomTown = "Random Town";
	const std::string RandomVillage = "Random Village";
	const std::vector<std::string> CityLocations =
	{
		ImperialMIF,
		RandomCity,
		RandomTown,
		RandomVillage
	};

	const std::string WildPlaceholderName = "WILD";
	const std::vector<std::string> WildernessLocations =
	{
		WildPlaceholderName
	};

	const std::string RandomNamedDungeon = "Random Named";
	const std::string RandomWildDungeon = "Random Wild";
	const std::vector<std::string> DungeonLocations =
	{
		RandomNamedDungeon,
		RandomWildDungeon
	};

	// Values for testing.
	const std::vector<ArenaTypes::WeatherType> Weathers =
	{
		ArenaTypes::WeatherType::Clear,
		ArenaTypes::WeatherType::Overcast,
		ArenaTypes::WeatherType::Rain,
		ArenaTypes::WeatherType::Snow,
		ArenaTypes::WeatherType::SnowOvercast,
		ArenaTypes::WeatherType::Rain2,
		ArenaTypes::WeatherType::Overcast2,
		ArenaTypes::WeatherType::SnowOvercast2
	};

	const std::unordered_map<ArenaTypes::WeatherType, std::string> WeatherTypeNames =
	{
		{ ArenaTypes::WeatherType::Clear, "Clear" },
		{ ArenaTypes::WeatherType::Overcast, "Overcast" },
		{ ArenaTypes::WeatherType::Rain, "Rain" },
		{ ArenaTypes::WeatherType::Snow, "Snow" },
		{ ArenaTypes::WeatherType::SnowOvercast, "Snow Overcast" },
		{ ArenaTypes::WeatherType::Rain2, "Rain 2" },
		{ ArenaTypes::WeatherType::Overcast2, "Overcast 2" },
		{ ArenaTypes::WeatherType::SnowOvercast2, "Snow Overcast 2" }
	};
}

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game)
{
	this->loadButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<LoadSavePanel>(game, LoadSavePanel::Type::Load);
		};

		return Button<Game&>(
			MainMenuUiView::LoadButtonCenterPoint,
			MainMenuUiView::LoadButtonWidth,
			MainMenuUiView::LoadButtonHeight,
			function);
	}();

	this->newButton = []()
	{
		auto function = [](Game &game)
		{
			// Link together the opening scroll, intro cinematic, and character creation.
			auto changeToCharCreation = [](Game &game)
			{
				game.setCharacterCreationState(std::make_unique<CharacterCreationState>());
				game.setPanel<ChooseClassCreationPanel>(game);

				const MusicLibrary &musicLibrary = game.getMusicLibrary();
				const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
					MusicDefinition::Type::CharacterCreation, game.getRandom());

				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing character creation music.");
				}

				AudioManager &audioManager = game.getAudioManager();
				audioManager.setMusic(musicDef);
			};

			auto changeToNewGameStory = [changeToCharCreation](Game &game)
			{
				std::vector<std::string> paletteNames
				{
					"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
					"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
					"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG"
				};

				std::vector<std::string> textureNames
				{
					"INTRO01.IMG", "INTRO02.IMG", "INTRO03.IMG",
					"INTRO04.IMG", "INTRO05.IMG", "INTRO06.IMG",
					"INTRO07.IMG", "INTRO08.IMG", "INTRO09.IMG"
				};

				std::vector<double> imageDurations
				{
					5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0
				};

				game.setPanel<ImageSequencePanel>(
					game,
					paletteNames,
					textureNames,
					imageDurations,
					changeToCharCreation);
			};

			game.setPanel<CinematicPanel>(
				game,
				ArenaTextureSequenceName::OpeningScroll,
				ArenaTextureSequenceName::OpeningScroll,
				1.0 / 24.0,
				changeToNewGameStory);

			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Cinematic, game.getRandom(), [](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Cinematic);
				const auto &cinematicMusicDef = def.getCinematicMusicDefinition();
				return cinematicMusicDef.type == MusicDefinition::CinematicMusicDefinition::Type::Intro;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing intro music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		};

		return Button<Game&>(
			MainMenuUiView::NewGameButtonCenterPoint,
			MainMenuUiView::NewGameButtonWidth,
			MainMenuUiView::NewGameButtonHeight,
			function);
	}();

	this->quickStartButton = [&game]()
	{
		auto function = [](Game &game, int testType, int testIndex, const std::string &mifName,
			const std::optional<ArenaTypes::InteriorType> &optInteriorType, ArenaTypes::WeatherType weatherType,
			MapType mapType)
		{
			// Initialize 3D renderer.
			auto &renderer = game.getRenderer();
			const auto &options = game.getOptions();
			const bool fullGameWindow = options.getGraphics_ModernInterface();
			renderer.initializeWorldRendering(options.getGraphics_ResolutionScale(),
				fullGameWindow, options.getGraphics_RenderThreadsMode());

			// Game data instance, to be initialized further by one of the loading methods below.
			// Create a player with random data for testing.
			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			auto gameState = std::make_unique<GameState>(Player::makeRandom(
				game.getCharacterClassLibrary(), binaryAssetLibrary.getExeData(), game.getRandom()),
				binaryAssetLibrary);

			const int starCount = SkyUtils::getStarCountFromDensity(options.getMisc_StarDensity());
			const int currentDay = gameState->getDate().getDay();

			// Load the selected level based on world type (writing into active game state).
			if (mapType == MapType::Interior)
			{
				if (testType != TestType_Dungeon)
				{
					const Player &player = gameState->getPlayer();
					const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();

					// Set some interior location data for testing, depending on whether it's a
					// main quest dungeon.
					int locationIndex, provinceIndex;
					if (testType == TestType_MainQuest)
					{
						// Fetch from a global function.
						const auto &exeData = game.getBinaryAssetLibrary().getExeData();
						SpecialCaseType specialCaseType;
						GetMainQuestLocationFromIndex(testIndex, exeData, &locationIndex, &provinceIndex, &specialCaseType);

						if (specialCaseType == SpecialCaseType::None)
						{
							// Do nothing.
						}
						else if (specialCaseType == SpecialCaseType::StartDungeon)
						{
							// @temp: hacky search for start dungeon location definition.
							const ProvinceDefinition &tempProvinceDef = worldMapDef.getProvinceDef(provinceIndex);
							for (int i = 0; i < tempProvinceDef.getLocationCount(); i++)
							{
								const LocationDefinition &curLocationDef = tempProvinceDef.getLocationDef(i);
								if (curLocationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
								{
									const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
										curLocationDef.getMainQuestDungeonDefinition();

									if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
									{
										locationIndex = i;
										break;
									}
								}
							}

							DebugAssertMsg(locationIndex != -1, "Couldn't find start dungeon location definition.");
						}
						else
						{
							DebugNotImplementedMsg(std::to_string(static_cast<int>(specialCaseType)));
						}
					}
					else
					{
						// Any province besides center province.
						// @temp: mildly disorganized
						provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
						locationIndex = GetRandomCityLocationIndex(worldMapDef.getProvinceDef(provinceIndex));
					}

					const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
					const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);

					DebugAssert(optInteriorType.has_value());
					const ArenaTypes::InteriorType interiorType = *optInteriorType;

					const std::optional<bool> rulerIsMale = [&locationDef]()
					{
						if (locationDef.getType() == LocationDefinition::Type::City)
						{
							const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
							return cityDef.rulerIsMale;
						}
						else
						{
							return false;
						}
					}();

					MapGeneration::InteriorGenInfo interiorGenInfo;
					interiorGenInfo.initPrefab(std::string(mifName), interiorType, rulerIsMale);

					const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest/interiors.

					const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);
					if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
						game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
						game.getBinaryAssetLibrary(), game.getTextureManager(), renderer))
					{
						DebugCrash("Couldn't load interior \"" + locationDef.getName() + "\".");
					}
				}
				else
				{
					// Pick a random dungeon based on the dungeon type.
					const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();

					const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
					const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

					constexpr bool isArtifactDungeon = false;

					const std::optional<VoxelInt2> playerStartOffset = VoxelInt2(
						ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
						ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

					if (mifName == RandomNamedDungeon)
					{
						const std::optional<int> locationIndex = GetRandomDungeonLocationDefIndex(provinceDef);
						DebugAssertMsg(locationIndex.has_value(), "Couldn't find named dungeon in \"" + provinceDef.getName() + "\".");

						const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
						const LocationDefinition::DungeonDefinition &dungeonDef = locationDef.getDungeonDefinition();

						MapGeneration::InteriorGenInfo interiorGenInfo;
						interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

						const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);
						if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
							game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
							game.getBinaryAssetLibrary(), game.getTextureManager(), game.getRenderer()))
						{
							DebugCrash("Couldn't load named dungeon \"" + locationDef.getName() + "\".");
						}

						// Set random named dungeon name and visibility for testing.
						LocationInstance &locationInst = gameState->getLocationInstance();
						locationInst.setNameOverride("Test Dungeon");

						if (!locationInst.isVisible())
						{
							locationInst.toggleVisibility();
						}
					}
					else if (mifName == RandomWildDungeon)
					{
						Random &random = game.getRandom();
						const int wildBlockX = random.next(ArenaWildUtils::WILD_WIDTH);
						const int wildBlockY = random.next(ArenaWildUtils::WILD_HEIGHT);

						const int locationIndex = GetRandomCityLocationIndex(provinceDef);
						const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
						const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

						const uint32_t dungeonSeed = cityDef.getWildDungeonSeed(wildBlockX, wildBlockY);
						constexpr int widthChunkCount = ArenaWildUtils::WILD_DUNGEON_WIDTH_CHUNKS;
						constexpr int heightChunkCount = ArenaWildUtils::WILD_DUNGEON_HEIGHT_CHUNKS;

						// Need to generate dungeon definition here since we don't have the wild chunk itself.
						LocationDefinition::DungeonDefinition dungeonDef;
						dungeonDef.init(dungeonSeed, widthChunkCount, heightChunkCount);

						MapGeneration::InteriorGenInfo interiorGenInfo;
						interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

						const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);
						if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
							game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
							binaryAssetLibrary, game.getTextureManager(), renderer))
						{
							DebugCrash("Couldn't load wilderness dungeon \"" + locationDef.getName() + "\".");
						}
					}
					else
					{
						DebugCrash("Unrecognized dungeon type \"" + mifName + "\".");
					}
				}
			}
			else if (mapType == MapType::City)
			{
				// There is only one "premade" city (used by the center province). All others
				// are randomly generated.
				// @todo: the IMPERIAL.MIF and random city/town/village branches could be merged a bit.
				if (mifName == ImperialMIF)
				{
					// Load city into game state.
					const int provinceIndex = LocationUtils::CENTER_PROVINCE_ID;
					const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
					const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
					const std::optional<int> locationIndex = [&mifName, &provinceDef]() -> std::optional<int>
					{
						for (int i = 0; i < provinceDef.getLocationCount(); i++)
						{
							const LocationDefinition &curLocationDef = provinceDef.getLocationDef(i);
							if (curLocationDef.getType() == LocationDefinition::Type::City)
							{
								const LocationDefinition::CityDefinition &cityDef = curLocationDef.getCityDefinition();
								if ((cityDef.type == ArenaTypes::CityType::CityState) && cityDef.premade &&
									cityDef.palaceIsMainQuestDungeon)
								{
									return i;
								}
							}
						}

						return std::nullopt;
					}();

					DebugAssertMsg(locationIndex.has_value(), "Couldn't find premade city with main quest palace dungeon.");
					const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);

					const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
					Buffer<uint8_t> reservedBlocks = [&cityDef]()
					{
						const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
						DebugAssert(cityReservedBlocks != nullptr);
						Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
						std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
						return buffer;
					}();

					const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
						[&cityDef]() -> std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
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
						mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

					const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
					{
						WeatherDefinition weatherDef;
						weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
						return weatherDef;
					}();

					SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
					skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
						cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

					const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);
					if (!gameState->trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
						game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), binaryAssetLibrary,
						game.getTextAssetLibrary(), game.getTextureManager(), renderer))
					{
						DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
					}
				}
				else
				{
					// Pick a random location based on the .MIF name, excluding the center province.
					const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
					const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
					const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

					const ArenaTypes::CityType targetCityType = [&mifName]()
					{
						if (mifName == RandomCity)
						{
							return ArenaTypes::CityType::CityState;
						}
						else if (mifName == RandomTown)
						{
							return ArenaTypes::CityType::Town;
						}
						else if (mifName == RandomVillage)
						{
							return ArenaTypes::CityType::Village;
						}
						else
						{
							DebugUnhandledReturnMsg(ArenaTypes::CityType, mifName);
						}
					}();

					const std::optional<int> locationIndex = GetRandomCityLocationDefIndexIfType(provinceDef, targetCityType);
					DebugAssertMsg(locationIndex.has_value(), "Couldn't find city for \"" + mifName + "\".");

					const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
					const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

					Buffer<uint8_t> reservedBlocks = [&cityDef]()
					{
						const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
						DebugAssert(cityReservedBlocks != nullptr);
						Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
						std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
						return buffer;
					}();

					const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
						[&cityDef]() -> std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
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
						mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

					const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
					{
						WeatherDefinition weatherDef;
						weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
						return weatherDef;
					}();

					SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
					skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
						cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

					const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

					// Load city into game state.
					if (!gameState->trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
						game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), binaryAssetLibrary,
						game.getTextAssetLibrary(), game.getTextureManager(), renderer))
					{
						DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
					}
				}
			}
			else if (mapType == MapType::Wilderness)
			{
				// Pick a random location and province.
				const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
				const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
				const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

				const int locationIndex = GetRandomCityLocationIndex(provinceDef);
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
				const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

				const auto &exeData = binaryAssetLibrary.getExeData();
				Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs =
					ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

				MapGeneration::WildGenInfo wildGenInfo;
				wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

				// Use current weather.
				const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
				{
					WeatherDefinition weatherDef;
					weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
					return weatherDef;
				}();

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// No previous start coordinate available. Let the loader decide.
				const std::optional<CoordInt3> startCoord;

				// Load wilderness into game state.
				const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);
				if (!gameState->trySetWilderness(wildGenInfo, skyGenInfo, overrideWeather, startCoord,
					worldMapLocationIDs, game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
					binaryAssetLibrary, game.getTextureManager(), renderer))
				{
					DebugCrash("Couldn't load wilderness \"" + locationDef.getName() + "\".");
				}
			}
			else
			{
				DebugCrash("Unrecognized world type \"" + 
					std::to_string(static_cast<int>(mapType)) + "\".");
			}

			// Set clock to 5:45am.
			auto &clock = gameState->getClock();
			clock = Clock(5, 45, 0);

			// Get the music that should be active on start.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = [&game, &mifName, &optInteriorType, mapType, &gameState, &musicLibrary]()
			{
				const bool isExterior = (mapType == MapType::City) || (mapType == MapType::Wilderness);

				// Exteriors depend on the time of day for which music to use. Interiors depend
				// on the current location's .MIF name (if any).
				if (isExterior)
				{
					if (!gameState->nightMusicIsActive())
					{
						const WeatherDefinition &weatherDef = gameState->getWeatherDefinition();
						return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
							game.getRandom(), [weatherDef](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Weather);
							const auto &weatherMusicDef = def.getWeatherMusicDefinition();
							return weatherMusicDef.weatherDef == weatherDef;
						});
					}
					else
					{
						return musicLibrary.getRandomMusicDefinition(
							MusicDefinition::Type::Night, game.getRandom());
					}
				}
				else
				{
					DebugAssert(optInteriorType.has_value());
					const MusicDefinition::InteriorMusicDefinition::Type interiorMusicType =
						MusicUtils::getInteriorMusicType(*optInteriorType);
					
					return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Interior,
						game.getRandom(), [interiorMusicType](const MusicDefinition &def)
					{
						DebugAssert(def.getType() == MusicDefinition::Type::Interior);
						const auto &interiorMusicDef = def.getInteriorMusicDefinition();
						return interiorMusicDef.type == interiorMusicType;
					});
				}
			}();

			const MusicDefinition *jingleMusicDef = [&game, mapType, &gameState, &musicLibrary]()
				-> const MusicDefinition*
			{
				const LocationDefinition &locationDef = gameState->getLocationDefinition();
				const bool isCity = (mapType == MapType::City) &&
					(locationDef.getType() == LocationDefinition::Type::City);

				if (isCity)
				{
					const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
					return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
						game.getRandom(), [&cityDef](const MusicDefinition &def)
					{
						DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
						const auto &jingleMusicDef = def.getJingleMusicDefinition();
						return (jingleMusicDef.cityType == cityDef.type) &&
							(jingleMusicDef.climateType == cityDef.climateType);
					});
				}
				else
				{
					return nullptr;
				}
			}();

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing start music.");
			}

			// Set the game state before constructing the game world panel.
			game.setGameState(std::move(gameState));

			// Initialize game world panel.
			game.setPanel<GameWorldPanel>(game);

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef, jingleMusicDef);
		};

		return Button<Game&, int, int, const std::string&,
			const std::optional<ArenaTypes::InteriorType>&, ArenaTypes::WeatherType, MapType>(function);
	}();

	this->exitButton = []()
	{
		auto function = []()
		{
			SDL_Event e;
			e.quit.type = SDL_QUIT;
			e.quit.timestamp = 0;
			SDL_PushEvent(&e);
		};

		return Button<>(
			MainMenuUiView::ExitButtonCenterPoint,
			MainMenuUiView::ExitButtonWidth,
			MainMenuUiView::ExitButtonHeight,
			function);
	}();

	this->testTypeUpButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			panel.testType = (panel.testType > 0) ? (panel.testType - 1) : (MaxTestTypes - 1);

			// Reset the other indices.
			panel.testIndex = 0;
			panel.testIndex2 = 1;
			panel.testWeather = 0;
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestTypeUpButtonX,
			MainMenuUiView::TestTypeUpButtonY,
			MainMenuUiView::TestTypeUpButtonWidth,
			MainMenuUiView::TestTypeUpButtonHeight,
			function);
	}();

	this->testTypeDownButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			panel.testType = (panel.testType < (MaxTestTypes - 1)) ? (panel.testType + 1) : 0;

			// Reset the other indices.
			panel.testIndex = 0;
			panel.testIndex2 = 1;
			panel.testWeather = 0;
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestTypeDownButtonX,
			MainMenuUiView::TestTypeDownButtonY,
			MainMenuUiView::TestTypeDownButtonWidth,
			MainMenuUiView::TestTypeDownButtonHeight,
			function);
	}();

	this->testIndexUpButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			const int count = [&panel]()
			{
				// Check test type to determine the max.
				if (panel.testType == TestType_MainQuest)
				{
					return MainQuestLocationCount;
				}
				else if (panel.testType == TestType_Interior)
				{
					return static_cast<int>(InteriorLocations.size());
				}
				else if (panel.testType == TestType_City)
				{
					return static_cast<int>(CityLocations.size());
				}
				else if (panel.testType == TestType_Wilderness)
				{
					return static_cast<int>(WildernessLocations.size());
				}
				else
				{
					return static_cast<int>(DungeonLocations.size());
				}
			}();

			panel.testIndex = (panel.testIndex > 0) ? (panel.testIndex - 1) : (count - 1);

			if (panel.testType == TestType_Interior)
			{
				// Reset the second index.
				panel.testIndex2 = 1;
			}
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestIndexUpButtonX,
			MainMenuUiView::TestIndexUpButtonY,
			MainMenuUiView::TestIndexUpButtonWidth,
			MainMenuUiView::TestIndexUpButtonHeight,
			function);
	}();

	this->testIndexDownButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			const int count = [&panel]()
			{
				// Check test type to determine the max.
				if (panel.testType == TestType_MainQuest)
				{
					return MainQuestLocationCount;
				}
				else if (panel.testType == TestType_Interior)
				{
					return static_cast<int>(InteriorLocations.size());
				}
				else if (panel.testType == TestType_City)
				{
					return static_cast<int>(CityLocations.size());
				}
				else if (panel.testType == TestType_Wilderness)
				{
					return static_cast<int>(WildernessLocations.size());
				}
				else
				{
					return static_cast<int>(DungeonLocations.size());
				}
			}();

			panel.testIndex = (panel.testIndex < (count - 1)) ? (panel.testIndex + 1) : 0;

			if (panel.testType == TestType_Interior)
			{
				// Reset the second index.
				panel.testIndex2 = 1;
			}
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestIndexDownButtonX,
			MainMenuUiView::TestIndexDownButtonY,
			MainMenuUiView::TestIndexDownButtonWidth,
			MainMenuUiView::TestIndexDownButtonHeight,
			function);
	}();

	this->testIndex2UpButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert(panel.testType == TestType_Interior);

			// Interior range.
			const auto &interior = InteriorLocations.at(panel.testIndex);
			const int minIndex = std::get<1>(interior).first;
			const int maxIndex = std::get<1>(interior).second;

			panel.testIndex2 = (panel.testIndex2 < maxIndex) ? (panel.testIndex2 + 1) : minIndex;
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestIndex2UpButtonX,
			MainMenuUiView::TestIndex2UpButtonY, 
			MainMenuUiView::TestIndex2UpButtonWidth,
			MainMenuUiView::TestIndex2UpButtonHeight,
			function);
	}();

	this->testIndex2DownButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert(panel.testType == TestType_Interior);

			// Interior range.
			const auto &interior = InteriorLocations.at(panel.testIndex);
			const int minIndex = std::get<1>(interior).first;
			const int maxIndex = std::get<1>(interior).second;

			panel.testIndex2 = (panel.testIndex2 > minIndex) ? (panel.testIndex2 - 1) : maxIndex;
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestIndex2DownButtonX,
			MainMenuUiView::TestIndex2DownButtonY,
			MainMenuUiView::TestIndex2DownButtonWidth,
			MainMenuUiView::TestIndex2DownButtonHeight,
			function);
	}();

	this->testWeatherUpButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert((panel.testType == TestType_City) || (panel.testType == TestType_Wilderness));

			panel.testWeather = [&panel]()
			{
				const int count = static_cast<int>(Weathers.size());
				return (panel.testWeather > 0) ? (panel.testWeather - 1) : (count - 1);
			}();
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestWeatherUpButtonX,
			MainMenuUiView::TestWeatherUpButtonY,
			MainMenuUiView::TestWeatherUpButtonWidth,
			MainMenuUiView::TestWeatherUpButtonHeight,
			function);
	}();

	this->testWeatherDownButton = []()
	{
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert((panel.testType == TestType_City) || (panel.testType == TestType_Wilderness));

			panel.testWeather = [&panel]()
			{
				const int count = static_cast<int>(Weathers.size());
				return (panel.testWeather < (count - 1)) ? (panel.testWeather + 1) : 0;
			}();
		};

		return Button<MainMenuPanel&>(
			MainMenuUiView::TestWeatherDownButtonX,
			MainMenuUiView::TestWeatherDownButtonY,
			MainMenuUiView::TestWeatherDownButtonWidth,
			MainMenuUiView::TestWeatherDownButtonHeight,
			function);
	}();

	this->testType = 0;
	this->testIndex = 0;
	this->testIndex2 = 1;
	this->testWeather = 0;

	// The game state should not be active on the main menu.
	DebugAssert(!game.gameStateIsActive());
}

std::string MainMenuPanel::getSelectedTestName() const
{
	if (this->testType == TestType_MainQuest)
	{
		auto &game = this->getGame();
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		const auto &exeData = binaryAssetLibrary.getExeData();

		// Decide how to get the main quest dungeon name.
		if (this->testIndex == 0)
		{
			// Start dungeon.
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if (this->testIndex == (MainQuestLocationCount - 1))
		{
			// Final dungeon.
			return String::toUppercase(exeData.locations.finalDungeonMifName);
		}
		else
		{
			// Generate the location from the executable data, fetching data from a
			// global function.
			int locationID, provinceID;
			SpecialCaseType specialCaseType;
			GetMainQuestLocationFromIndex(this->testIndex, exeData, &locationID, &provinceID, &specialCaseType);
			DebugAssert(specialCaseType == SpecialCaseType::None);

			// Calculate the .MIF name from the dungeon seed.
			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const uint32_t dungeonSeed = [&cityData, locationID, provinceID]()
			{
				const auto &province = cityData.getProvinceData(provinceID);
				const int localDungeonID = locationID - 32;
				return LocationUtils::getDungeonSeed(localDungeonID, provinceID, province);
			}();

			const std::string mifName = LocationUtils::getMainQuestDungeonMifName(dungeonSeed);
			return String::toUppercase(mifName);
		}
	}
	else if (this->testType == TestType_Interior)
	{
		const auto &interior = InteriorLocations.at(this->testIndex);
		return std::get<0>(interior) + std::to_string(this->testIndex2) + ".MIF";
	}
	else if (this->testType == TestType_City)
	{
		return CityLocations.at(this->testIndex);
	}
	else if (this->testType == TestType_Wilderness)
	{
		return WildernessLocations.at(this->testIndex);
	}
	else
	{
		return DungeonLocations.at(this->testIndex);
	}
}

std::optional<ArenaTypes::InteriorType> MainMenuPanel::getSelectedTestInteriorType() const
{
	if (this->testType == TestType_MainQuest || this->testType == TestType_Dungeon)
	{
		return ArenaTypes::InteriorType::Dungeon;
	}
	else if (this->testType == TestType_Interior)
	{
		const auto &interior = InteriorLocations.at(this->testIndex);
		return std::get<2>(interior);
	}
	else if (this->testType == TestType_City || this->testType == TestType_Wilderness)
	{
		return std::nullopt;
	}
	else
	{
		DebugCrash("Unimplemented test type \"" + std::to_string(this->testType) + "\".");
		return std::nullopt;
	}
}

ArenaTypes::WeatherType MainMenuPanel::getSelectedTestWeatherType() const
{
	return Weathers.at(this->testWeather);
}

MapType MainMenuPanel::getSelectedTestMapType() const
{
	if ((this->testType == TestType_MainQuest) ||
		(this->testType == TestType_Interior) ||
		(this->testType == TestType_Dungeon))
	{
		return MapType::Interior;
	}
	else if (this->testType == TestType_City)
	{
		return MapType::City;
	}
	else
	{
		return MapType::Wilderness;
	}
}

std::optional<Panel::CursorData> MainMenuPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void MainMenuPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool lPressed = inputManager.keyPressed(e, SDLK_l);
	bool sPressed = inputManager.keyPressed(e, SDLK_s);
	bool ePressed = inputManager.keyPressed(e, SDLK_e);
	bool fPressed = inputManager.keyPressed(e, SDLK_f);

	if (lPressed)
	{
		this->loadButton.click(this->getGame());
	}
	else if (sPressed)
	{
		this->newButton.click(this->getGame());
	}
	else if (ePressed)
	{
		this->exitButton.click();
	}
	else if (fPressed)
	{
		// Enter the game world immediately (for testing purposes). Use the test traits
		// selected on the main menu.
		this->quickStartButton.click(this->getGame(),
			this->testType,
			this->testIndex,
			this->getSelectedTestName(),
			this->getSelectedTestInteriorType(),
			this->getSelectedTestWeatherType(),
			this->getSelectedTestMapType());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->loadButton.contains(originalPoint))
		{
			this->loadButton.click(this->getGame());
		}
		else if (this->newButton.contains(originalPoint))
		{
			this->newButton.click(this->getGame());
		}
		else if (this->exitButton.contains(originalPoint))
		{
			this->exitButton.click();
		}
		else if (MainMenuUiView::TestButtonRect.contains(originalPoint))
		{
			// Enter the game world immediately (for testing purposes). Use the test traits
			// selected on the main menu.
			this->quickStartButton.click(this->getGame(),
				this->testType,
				this->testIndex,
				this->getSelectedTestName(),
				this->getSelectedTestInteriorType(),
				this->getSelectedTestWeatherType(),
				this->getSelectedTestMapType());
		}
		else if (this->testTypeUpButton.contains(originalPoint))
		{
			this->testTypeUpButton.click(*this);
		}
		else if (this->testTypeDownButton.contains(originalPoint))
		{
			this->testTypeDownButton.click(*this);
		}
		else if (this->testIndexUpButton.contains(originalPoint))
		{
			this->testIndexUpButton.click(*this);
		}
		else if (this->testIndexDownButton.contains(originalPoint))
		{
			this->testIndexDownButton.click(*this);
		}
		else if (this->testType == TestType_Interior)
		{
			// These buttons are only available when selecting interior names.
			if (this->testIndex2UpButton.contains(originalPoint))
			{
				this->testIndex2UpButton.click(*this);
			}
			else if (this->testIndex2DownButton.contains(originalPoint))
			{
				this->testIndex2DownButton.click(*this);
			}
		}
		else if (this->testType == TestType_City)
		{
			// These buttons are only available when selecting city names.
			if (this->testWeatherUpButton.contains(originalPoint))
			{
				this->testWeatherUpButton.click(*this);
			}
			else if (this->testWeatherDownButton.contains(originalPoint))
			{
				this->testWeatherDownButton.click(*this);
			}
		}
		else if (this->testType == TestType_Wilderness)
		{
			// These buttons are only available when selecting wilderness names.
			if (this->testWeatherUpButton.contains(originalPoint))
			{
				this->testWeatherUpButton.click(*this);
			}
			else if (this->testWeatherDownButton.contains(originalPoint))
			{
				this->testWeatherDownButton.click(*this);
			}
		}
	}
}

void MainMenuPanel::renderTestUI(Renderer &renderer)
{
	// Draw test buttons.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference &arrowsPaletteTextureAssetRef = MainMenuUiView::getTestArrowsPaletteTextureAssetRef();
	const std::optional<PaletteID> arrowsPaletteID = textureManager.tryGetPaletteID(arrowsPaletteTextureAssetRef);
	if (!arrowsPaletteID.has_value())
	{
		DebugLogError("Couldn't get arrows palette ID for \"" + arrowsPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference &arrowsTextureAssetRef = MainMenuUiView::getTestArrowsTextureAssetRef();
	const std::optional<TextureBuilderID> arrowsTextureBuilderID = textureManager.tryGetTextureBuilderID(arrowsTextureAssetRef);
	if (!arrowsTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get arrows texture builder ID for \"" + arrowsTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
		this->testTypeUpButton.getX(), this->testTypeUpButton.getY(), textureManager);
	renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
		this->testIndexUpButton.getX(), this->testIndexUpButton.getY(), textureManager);

	if (this->testType == TestType_Interior)
	{
		renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
			this->testIndex2UpButton.getX(), this->testIndex2UpButton.getY(), textureManager);
	}
	else if ((this->testType == TestType_City) || (this->testType == TestType_Wilderness))
	{
		renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
			this->testWeatherUpButton.getX(), this->testWeatherUpButton.getY(), textureManager);
	}

	const Rect &testButtonRect = MainMenuUiView::TestButtonRect;
	const Texture testButton = TextureUtils::generate(MainMenuUiView::TestButtonPatternType,
		testButtonRect.getWidth(), testButtonRect.getHeight(), textureManager, renderer);
	renderer.drawOriginal(testButton, testButtonRect.getLeft(), testButtonRect.getTop(),
		testButton.getWidth(), testButton.getHeight());

	// Draw test text.
	const auto &fontLibrary = this->getGame().getFontLibrary();
	const RichTextString testButtonText(
		"Test",
		MainMenuUiView::TestButtonFontName,
		MainMenuUiView::getTestButtonTextColor(),
		MainMenuUiView::TestButtonTextAlignment,
		fontLibrary);

	const TextBox testButtonTextBox(MainMenuUiView::TestButtonTextBoxPoint, testButtonText, fontLibrary, renderer);
	renderer.drawOriginal(testButtonTextBox.getTexture(), testButtonTextBox.getX(), testButtonTextBox.getY());

	const std::string testTypeName = GetTestTypeName(this->testType);
	const RichTextString testTypeText(
		"Test type: " + testTypeName,
		testButtonText.getFontName(),
		testButtonText.getColor(),
		TextAlignment::Left,
		fontLibrary);

	const int testTypeTextBoxX = this->testTypeUpButton.getX() - testTypeText.getDimensions().x - 2;
	const int testTypeTextBoxY = this->testTypeUpButton.getY() + (testTypeText.getDimensions().y / 2);
	const TextBox testTypeTextBox(testTypeTextBoxX, testTypeTextBoxY, testTypeText, fontLibrary, renderer);
	renderer.drawOriginal(testTypeTextBox.getTexture(), testTypeTextBox.getX(), testTypeTextBox.getY());

	const RichTextString testNameText(
		"Test location: " + this->getSelectedTestName(),
		testTypeText.getFontName(),
		testTypeText.getColor(),
		testTypeText.getAlignment(),
		fontLibrary);
	const int testNameTextBoxX = this->testIndexUpButton.getX() - testNameText.getDimensions().x - 2;
	const int testNameTextBoxY = this->testIndexUpButton.getY() + (testNameText.getDimensions().y / 2);
	const TextBox testNameTextBox(testNameTextBoxX, testNameTextBoxY, testNameText, fontLibrary, renderer);
	renderer.drawOriginal(testNameTextBox.getTexture(), testNameTextBox.getX(), testNameTextBox.getY());

	// Draw weather text if applicable.
	if ((this->testType == TestType_City) || (this->testType == TestType_Wilderness))
	{
		const ArenaTypes::WeatherType weatherType = this->getSelectedTestWeatherType();
		const std::string &weatherName = WeatherTypeNames.at(weatherType);

		const RichTextString testWeatherText(
			"Test weather: " + weatherName,
			testTypeText.getFontName(),
			testTypeText.getColor(),
			testTypeText.getAlignment(),
			fontLibrary);

		const int testWeatherTextBoxX = this->testWeatherUpButton.getX() - testWeatherText.getDimensions().x - 2;
		const int testWeatherTextBoxY = this->testWeatherUpButton.getY() + (testWeatherText.getDimensions().y / 2);
		const TextBox testWeatherTextBox(testWeatherTextBoxX, testWeatherTextBoxY,
			testWeatherText, fontLibrary, renderer);

		renderer.drawOriginal(testWeatherTextBox.getTexture(), testWeatherTextBox.getX(), testWeatherTextBox.getY());
	}
}

void MainMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw main menu.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference &backgroundPaletteTextureAssetRef = MainMenuUiView::getPaletteTextureAssetRef();
	const std::optional<PaletteID> mainMenuPaletteID = textureManager.tryGetPaletteID(backgroundPaletteTextureAssetRef);
	if (!mainMenuPaletteID.has_value())
	{
		DebugLogError("Couldn't get main menu palette ID for \"" + backgroundPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference &backgroundTextureAssetRef = MainMenuUiView::getBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> mainMenuTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!mainMenuTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get main menu texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*mainMenuTextureBuilderID, *mainMenuPaletteID, textureManager);
	this->renderTestUI(renderer);
}
