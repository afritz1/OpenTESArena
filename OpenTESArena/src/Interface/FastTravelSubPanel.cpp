#include <algorithm>
#include <sstream>

#include "CursorAlignment.h"
#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "TextAlignment.h"
#include "TextSubPanel.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/MIFFile.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Media/FontName.h"
#include "../Media/MusicUtils.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../World/LocationDefinition.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/WeatherUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

const double FastTravelSubPanel::FRAME_TIME = 1.0 / 24.0;
const double FastTravelSubPanel::MIN_SECONDS = 1.0;

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
	return TextureFile::fromName(TextureName::WorldMap);
}

TextureManager::IdGroup<TextureID> FastTravelSubPanel::getAnimationTextureIDs() const
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const std::string &paletteFilename = this->getBackgroundFilename();
	PaletteID paletteID;
	if (!textureManager.tryGetPaletteID(paletteFilename.c_str(), &paletteID))
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const std::string &textureFilename = TextureFile::fromName(TextureName::FastTravel);
	TextureManager::IdGroup<TextureID> textureIDs;
	if (!textureManager.tryGetTextureIDs(textureFilename.c_str(), paletteID, renderer, &textureIDs))
	{
		DebugCrash("Couldn't get texture IDs for \"" + textureFilename + "\".");
	}

	return textureIDs;
}

std::unique_ptr<Panel> FastTravelSubPanel::makeCityArrivalPopUp() const
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	const Int2 center = GameWorldPanel::getInterfaceCenter(
		modernInterface, textureManager, renderer) - Int2(0, 1);

	const std::string text = [this, &game]()
	{
		auto &gameData = game.getGameData();
		const auto &miscAssets = game.getMiscAssets();
		const auto &exeData = miscAssets.getExeData();

		// @todo: change these to 'index' so it's clear they don't rely on original game's 0-32, etc..
		const int provinceID = this->travelData.provinceID;
		const int localCityID = this->travelData.locationID;

		const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
		const LocationDefinition &locationDef = provinceDef.getLocationDef(localCityID);

		const std::string locationString = [this, &gameData, &exeData, provinceID, localCityID,
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

		const std::string dateString = [&gameData, &exeData]()
		{
			return exeData.travel.arrivalPopUpDate +
				GameData::getDateString(gameData.getDate(), exeData);
		}();

		const std::string daysString = [this, &exeData]()
		{
			std::string text = exeData.travel.arrivalPopUpDays;

			// Replace %d with travel days.
			size_t index = text.find("%d");
			text.replace(index, 2, std::to_string(this->travelData.travelDays));

			return text;
		}();

		const std::string locationDescriptionString = [&game, &gameData, &exeData,
			provinceID, localCityID, &locationDef]()
		{
			const auto &miscAssets = game.getMiscAssets();
			const LocationType locationType = LocationUtils::getCityType(localCityID);

			// Get the description for the local location. If it's a town or village, choose
			// one of the three substrings randomly. Otherwise, get the city description text
			// directly.
			const std::string description = [&gameData, &exeData, provinceID, localCityID,
				&locationDef, locationType, &miscAssets]()
			{
				// City descriptions start at #0600. The three town descriptions are at #1422,
				// and the three village descriptions are at #1423.
				const std::vector<std::string> &templateDatTexts = [provinceID, localCityID,
					locationType, &miscAssets]()
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

					const auto &templateDat = miscAssets.getTemplateDat();
					const auto &entry = templateDat.getEntry(key);
					return entry.values;
				}();

				if (locationType == LocationType::CityState)
				{
					return templateDatTexts.front();
				}
				else
				{
					ArenaRandom &random = gameData.getRandom();
					std::string description = [&random, &templateDatTexts]()
					{
						return templateDatTexts.at(random.next() % templateDatTexts.size());
					}();

					// Replace %cn with city name.
					size_t index = description.find("%cn");
					description.replace(index, 3, locationDef.getName());

					const auto &cityData = miscAssets.getCityDataFile();
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
						const std::string &rulerTitle = miscAssets.getRulerTitle(
							provinceID, locationType, isMale, random);
						description.replace(index, 2, rulerTitle);
					}

					// Replace %rf with ruler first name (if it exists). Make sure to reset
					// the random seed.
					random.srand(rulerSeed);
					index = description.find("%rf");
					if (index != std::string::npos)
					{
						const std::string rulerFirstName = [&miscAssets, provinceID,
							&random, isMale]()
						{
							const std::string fullName = miscAssets.generateNpcName(
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
		game.getFontManager());

	Texture texture = Texture::generate(Texture::PatternType::Dark,
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

Panel::CursorData FastTravelSubPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void FastTravelSubPanel::tickTravelTime(Random &random) const
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();

	// Tick the game date by the number of travel days.
	auto &date = gameData.getDate();
	for (int i = 0; i < this->travelData.travelDays; i++)
	{
		date.incrementDay();
	}

	// Add between 0 and 22 random hours to the clock time.
	auto &clock = gameData.getClock();
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
	auto &gameData = game.getGameData();
	const auto &miscAssets = game.getMiscAssets();
	const auto &exeData = miscAssets.getExeData();
	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();

	// Update game clock.
	this->tickTravelTime(game.getRandom());

	// Update weathers.
	gameData.updateWeather(exeData);

	// Clear the lore text (action text and effect text are unchanged).
	gameData.resetTriggerText();

	// Clear any on-voxel-enter event to avoid things like fast travelling out of the
	// starting dungeon then being teleported to a random city when going through any
	// subsequent LEVELUP voxel.
	gameData.getOnLevelUpVoxelEnter() = std::function<void(Game&)>();

	// Pop this sub-panel on the next game loop. The game loop pops old sub-panels before
	// pushing new ones, so call order doesn't matter.
	game.popSubPanel();

	const auto &travelProvinceDef = worldMapDef.getProvinceDef(this->travelData.provinceID);
	const auto &travelLocationDef = travelProvinceDef.getLocationDef(this->travelData.locationID);

	// Decide how to load the location.
	if (travelLocationDef.getType() == LocationDefinition::Type::City)
	{
		// Get weather type from game data.
		const LocationDefinition::CityDefinition &cityDef = travelLocationDef.getCityDefinition();
		const WeatherType weatherType = [this, &game, &gameData, &miscAssets,
			&travelProvinceDef, &travelLocationDef, &cityDef]()
		{
			const Int2 localPoint(travelLocationDef.getScreenX(), travelLocationDef.getScreenY());
			const Int2 globalPoint = LocationUtils::getGlobalPoint(
				localPoint, travelProvinceDef.getGlobalRect());

			const auto &cityData = miscAssets.getCityDataFile();
			const int globalQuarter = LocationUtils::getGlobalQuarter(globalPoint, cityData);

			const auto &weathersArray = gameData.getWeathersArray();
			DebugAssertIndex(weathersArray, globalQuarter);
			return WeatherUtils::getFilteredWeatherType(weathersArray[globalQuarter], cityDef.climateType);
		}();

		const int starCount = DistantSky::getStarCountFromDensity(
			game.getOptions().getMisc_StarDensity());

		// Load the destination city.
		if (!gameData.loadCity(travelLocationDef, travelProvinceDef, weatherType, starCount,
			game.getMiscAssets(), game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load city \"" + travelLocationDef.getName() + "\".");
		}

		// Choose time-based music and enter the game world.
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = [&game, &gameData, weatherType, &musicLibrary]()
		{
			if (!gameData.nightMusicIsActive())
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

		game.setMusic(musicDef, jingleMusicDef);
		game.setPanel<GameWorldPanel>(game);

		// Push a text sub-panel for the city arrival pop-up.
		std::unique_ptr<Panel> arrivalPopUp = this->makeCityArrivalPopUp();
		game.pushSubPanel(std::move(arrivalPopUp));
	}
	else if (travelLocationDef.getType() == LocationDefinition::Type::Dungeon)
	{
		// Random named dungeon.
		const bool isArtifactDungeon = false;
		const auto &travelProvinceDef = worldMapDef.getProvinceDef(this->travelData.provinceID);
		const auto &travelLocationDef = travelProvinceDef.getLocationDef(this->travelData.locationID);

		if (!gameData.loadNamedDungeon(travelLocationDef, travelProvinceDef, isArtifactDungeon,
			VoxelDefinition::WallData::MenuType::Dungeon, miscAssets,
			game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load named dungeon \"" + travelLocationDef.getName() + "\".");
		}

		// Choose random dungeon music and enter game world.
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
			MusicDefinition::Type::Dungeon, game.getRandom());

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing dungeon music.");
		}

		game.setMusic(musicDef);
		game.setPanel<GameWorldPanel>(game);
	}
	else if (travelLocationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
	{
		// Main quest dungeon. The staff dungeons have a splash image before going
		// to the game world panel.
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
			travelLocationDef.getMainQuestDungeonDefinition();
		const std::string mifName = mainQuestDungeonDef.mapFilename;

		MIFFile mif;
		if (!mif.init(mifName.c_str()))
		{
			DebugCrash("Could not init .MIF file \"" + mifName + "\".");
		}

		if (!gameData.loadInterior(travelLocationDef, travelProvinceDef,
			VoxelDefinition::WallData::MenuType::Dungeon, mif, miscAssets,
			game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load interior \"" + travelLocationDef.getName() + "\".");
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
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
				MusicDefinition::Type::Dungeon, game.getRandom());

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing dungeon music.");
			}

			game.setMusic(musicDef);
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

		if (this->frameIndex == this->getAnimationTextureIDs().count)
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
	const TextureManager::IdGroup<TextureID> animationTextureIDs = this->getAnimationTextureIDs();
	const TextureID animationTextureID =
		animationTextureIDs.startID + static_cast<int>(this->frameIndex);
	const TextureRef animFrame = textureManager.getTextureRef(animationTextureID);

	const int x = (Renderer::ORIGINAL_WIDTH / 2) - (animFrame.getWidth() / 2);
	const int y = (Renderer::ORIGINAL_HEIGHT / 2) - (animFrame.getHeight() / 2);
	renderer.drawOriginal(animFrame.get(), x, y);
}
