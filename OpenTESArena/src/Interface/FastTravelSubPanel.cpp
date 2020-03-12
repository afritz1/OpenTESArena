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
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../World/LocationType.h"

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

const std::vector<Texture> &FastTravelSubPanel::getAnimation() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const std::vector<Texture> &animation = textureManager.getTextures(
		TextureFile::fromName(TextureName::FastTravel),
		this->getBackgroundFilename(), renderer);
	return animation;
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
		const auto &exeData = game.getMiscAssets().getExeData();

		const auto &cityData = gameData.getCityDataFile();
		const int provinceID = this->travelData.provinceID;
		const int localCityID = this->travelData.locationID;
		const auto &provinceData = cityData.getProvinceData(provinceID);
		const auto &locationData = provinceData.getLocationData(localCityID);

		const std::string locationString = [this, &gameData, &exeData, &cityData,
			localCityID, &provinceData, &locationData]()
		{
			if (this->travelData.provinceID != Location::CENTER_PROVINCE_ID)
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
				text.replace(index, 2, locationData.name);

				// Replace third %s with province name.
				index = text.find("%s", index);
				text.replace(index, 2, provinceData.name);

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
			&cityData, provinceID, localCityID, &locationData]()
		{
			const auto &miscAssets = game.getMiscAssets();
			const LocationType locationType = Location::getCityType(localCityID);

			// Get the description for the local location. If it's a town or village, choose
			// one of the three substrings randomly. Otherwise, get the city description text
			// directly.
			const std::string description = [&gameData, &exeData, &cityData, provinceID,
				localCityID, &locationData, locationType, &miscAssets]()
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
					description.replace(index, 3, locationData.name);

					const uint32_t rulerSeed = cityData.getRulerSeed(localCityID, provinceID);
					const bool isMale = cityData.isRulerMale(localCityID, provinceID);

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
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return CursorData(&texture, CursorAlignment::TopLeft);
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

	// Update game clock.
	Random random;
	this->tickTravelTime(random);

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

	// Decide how to load the location.
	if (this->travelData.locationID < 32)
	{
		// Get weather type from game data.
		const WeatherType weatherType = [this, &game, &gameData]()
		{
			const auto &cityData = gameData.getCityDataFile();
			const auto &provinceData = cityData.getProvinceData(this->travelData.provinceID);
			const auto &locationData = provinceData.getLocationData(this->travelData.locationID);
			const Int2 localPoint(locationData.x, locationData.y);
			const Int2 globalPoint = CityDataFile::localPointToGlobal(
				localPoint, provinceData.getGlobalRect());
			const int globalQuarter = cityData.getGlobalQuarter(globalPoint);
			const WeatherType type = gameData.getWeathersArray().at(globalQuarter);
			const ClimateType climateType = Location::getCityClimateType(
				this->travelData.locationID, this->travelData.provinceID, game.getMiscAssets());
			return GameData::getFilteredWeatherType(type, climateType);
		}();

		const int starCount = DistantSky::getStarCountFromDensity(
			game.getOptions().getMisc_StarDensity());

		// Load the destination city. For the center province, use the specialized method.
		if (this->travelData.provinceID != Location::CENTER_PROVINCE_ID)
		{
			gameData.loadCity(this->travelData.locationID, this->travelData.provinceID,
				weatherType, starCount, game.getMiscAssets(), game.getTextureManager(), game.getRenderer());
		}
		else
		{
			const std::string mifName = String::toUppercase(
				exeData.locations.centerProvinceCityMifName);

			MIFFile mif;
			if (!mif.init(mifName.c_str()))
			{
				DebugCrash("Could not init .MIF file \"" + mifName + "\".");
			}

			gameData.loadPremadeCity(mif, weatherType, starCount, game.getMiscAssets(),
				game.getTextureManager(), game.getRenderer());
		}

		// Choose time-based music and enter the game world.
		const MusicName musicName = gameData.nightMusicIsActive() ?
			MusicName::Night : GameData::getExteriorMusicName(weatherType);
		game.setMusic(musicName);
		game.setPanel<GameWorldPanel>(game);

		// Push a text sub-panel for the city arrival pop-up.
		std::unique_ptr<Panel> arrivalPopUp = this->makeCityArrivalPopUp();
		game.pushSubPanel(std::move(arrivalPopUp));
	}
	else
	{
		const int localDungeonID = this->travelData.locationID - 32;

		if ((localDungeonID == 0) || (localDungeonID == 1))
		{
			// Main quest dungeon. The staff dungeons have a splash image before going
			// to the game world panel.
			const auto &cityData = gameData.getCityDataFile();
			const uint32_t dungeonSeed = cityData.getDungeonSeed(
				localDungeonID, this->travelData.provinceID);
			
			const std::string mifName = CityDataFile::getMainQuestDungeonMifName(dungeonSeed);
			MIFFile mif;
			if (!mif.init(mifName.c_str()))
			{
				DebugCrash("Could not init .MIF file \"" + mifName + "\".");
			}

			const Location location = Location::makeDungeon(
				localDungeonID, this->travelData.provinceID);
			gameData.loadInterior(VoxelDefinition::WallData::MenuType::Dungeon,
				mif, location, miscAssets, game.getTextureManager(), game.getRenderer());

			const bool isStaffDungeon = localDungeonID == 0;

			if (isStaffDungeon)
			{
				// Go to staff dungeon splash image first.
				game.setPanel<MainQuestSplashPanel>(game, this->travelData.provinceID);
			}
			else
			{
				// Choose random dungeon music and enter game world.
				const MusicName musicName = GameData::getDungeonMusicName(random);
				game.setMusic(musicName);
				game.setPanel<GameWorldPanel>(game);
			}
		}
		else
		{
			// Random named dungeon.
			const bool isArtifactDungeon = false;
			gameData.loadNamedDungeon(localDungeonID, this->travelData.provinceID,
				isArtifactDungeon, VoxelDefinition::WallData::MenuType::Dungeon, miscAssets,
				game.getTextureManager(), game.getRenderer());

			// Choose random dungeon music and enter game world.
			const MusicName musicName = GameData::getDungeonMusicName(random);
			game.setMusic(musicName);
			game.setPanel<GameWorldPanel>(game);
		}
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

		if (this->frameIndex == this->getAnimation().size())
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
	const std::vector<Texture> &animation = this->getAnimation();
	const Texture &animFrame = animation.at(this->frameIndex);

	const int x = (Renderer::ORIGINAL_WIDTH / 2) - (animFrame.getWidth() / 2);
	const int y = (Renderer::ORIGINAL_HEIGHT / 2) - (animFrame.getHeight() / 2);
	renderer.drawOriginal(animFrame, x, y);
}
