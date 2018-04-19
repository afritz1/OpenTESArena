#include <algorithm>
#include <sstream>

#include "CursorAlignment.h"
#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "TextAlignment.h"
#include "TextSubPanel.h"
#include "../Assets/CityDataFile.h"
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
#include "../Utilities/String.h"

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

				// Replace carriage returns with newlines.
				text = String::replace(text, '\r', '\n');

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
			std::string description = [&gameData, &exeData, &cityData, provinceID, localCityID,
				&locationData, locationType, &miscAssets]()
			{
				// City descriptions start at #0600. The three town descriptions are at #1422,
				// and the three village descriptions are at #1423.
				const std::string &templateDatText = [provinceID, localCityID,
					locationType, &miscAssets]()
				{
					// Get the key that maps into TEMPLATE.DAT.
					// - To do: use an integer instead.
					const std::string key = [provinceID, localCityID,
						locationType]() -> std::string
					{
						if (locationType == LocationType::CityState)
						{
							std::stringstream ss;
							ss << std::setfill('0') << std::setw(2) <<
								std::to_string(localCityID + (8 * provinceID));
							return "#06" + ss.str();
						}
						else if (locationType == LocationType::Town)
						{
							return "#1422";
						}
						else if (locationType == LocationType::Village)
						{
							return "#1423";
						}
						else
						{
							throw std::runtime_error("Bad location type \"" +
								std::to_string(static_cast<int>(locationType)) + "\".");
						}
					}();

					return miscAssets.getTemplateDatText(key);
				}();

				if (locationType == LocationType::CityState)
				{
					return templateDatText;
				}
				else
				{
					ArenaRandom &random = gameData.getRandom();
					std::string description = [&random, &templateDatText]()
					{
						std::vector<std::string> strings = String::split(templateDatText, '&');
						return strings.at(random.next() % strings.size());
					}();

					// Replace %cn with city name.
					size_t index = description.find("%cn");
					description.replace(index, 3, locationData.name);

					const uint32_t rulerSeed = cityData.getRulerSeed(localCityID, provinceID);
					const bool isMale = (rulerSeed & 0x3) != 0;

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

					// Remove erroneous carriage returns and newlines at the beginning.
					// - To do: don't do clean-up on templateDat construction.
					while ((description.front() == '\r') || (description.front() == '\n'))
					{
						description.erase(description.begin());
					}

					return description;
				}
			}();

			// Remove erroneous carriage returns and newlines at the end (if any).
			if ((description.back() == '\r') || (description.back() == '\n'))
			{
				description.pop_back();
			}

			return description;
		}();

		// To do: re-distribute newlines based on max text box width.
		std::string fullText = locationString + dateString +
			daysString + '\n' + locationDescriptionString;

		fullText = String::replace(fullText, '\r', '\n');

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

	Texture texture(Texture::generate(Texture::PatternType::Dark,
		richText.getDimensions().x + 10, richText.getDimensions().y + 12,
		textureManager, renderer));

	const Int2 textureCenter(center.x, center.y + 1);

	auto function = [](Game &game)
	{
		game.popSubPanel();
	};

	return std::make_unique<TextSubPanel>(game, center, richText, function,
		std::move(texture), textureCenter);
}

std::pair<SDL_Texture*, CursorAlignment> FastTravelSubPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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

	// Update game clock.
	Random random;
	this->tickTravelTime(random);

	// Update weathers.
	gameData.updateWeather(game.getMiscAssets().getExeData());

	// Clear the lore text (action text and effect text are unchanged).
	gameData.getTriggerText().reset();

	// Pop this sub-panel on the next game loop. The game loop pops old sub-panels before
	// pushing new ones, so call order doesn't matter.
	game.popSubPanel();

	// Decide how to load the location.
	if (this->travelData.locationID < 32)
	{
		// Get weather type from game data.
		const WeatherType weatherType = [this, &gameData]()
		{
			const auto &cityData = gameData.getCityDataFile();
			const auto &provinceData = cityData.getProvinceData(this->travelData.provinceID);
			const auto &locationData = provinceData.getLocationData(this->travelData.locationID);
			const Int2 localPoint(locationData.x, locationData.y);
			const Int2 globalPoint = CityDataFile::localPointToGlobal(
				localPoint, provinceData.getGlobalRect());
			const int globalQuarter = cityData.getGlobalQuarter(globalPoint);
			return gameData.getWeathersArray().at(globalQuarter);
		}();

		// Load the destination city. For the center province, use the specialized method.
		if (this->travelData.provinceID != Location::CENTER_PROVINCE_ID)
		{
			gameData.loadCity(this->travelData.locationID, this->travelData.provinceID,
				weatherType, game.getMiscAssets(), game.getTextureManager(), game.getRenderer());
		}
		else
		{
			const auto &exeData = game.getMiscAssets().getExeData();
			const std::string mifName = String::toUppercase(
				exeData.locations.centerProvinceCityMifName);
			const MIFFile mif(mifName);
			gameData.loadPremadeCity(mif, weatherType, game.getMiscAssets(),
				game.getTextureManager(), game.getRenderer());
		}

		// Choose time-based music and enter the game world.
		auto &clock = gameData.getClock();
		const MusicName musicName = clock.nightMusicIsActive() ?
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
			const MIFFile mif(mifName);
			const Location location = Location::makeDungeon(
				localDungeonID, this->travelData.provinceID);
			gameData.loadInterior(mif, location, game.getTextureManager(), game.getRenderer());

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
				isArtifactDungeon, game.getTextureManager(), game.getRenderer());

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
	renderer.drawOriginal(animFrame.get(), x, y);
}
