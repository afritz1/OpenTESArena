#include <algorithm>

#include "CursorAlignment.h"
#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

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

void FastTravelSubPanel::switchToGameWorld()
{
	// Handle fast travel behavior.
	auto &game = this->getGame();
	auto &gameData = game.getGameData();

	// Tick the game date by the number of travel days.
	auto &date = gameData.getDate();
	for (int i = 0; i < this->travelData.travelDays; i++)
	{
		date.incrementDay();
	}

	// Add between 0 and 22 random hours to the clock time.
	Random random;
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

	// Update weathers.
	gameData.updateWeather(game.getMiscAssets().getExeData());

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
		if (this->travelData.provinceID != 8)
		{
			gameData.loadCity(this->travelData.locationID, this->travelData.provinceID,
				weatherType, game.getMiscAssets(), game.getTextureManager(), game.getRenderer());
		}
		else
		{
			const MIFFile mif("IMPERIAL.MIF");
			gameData.loadPremadeCity(mif, weatherType, game.getMiscAssets(),
				game.getTextureManager(), game.getRenderer());
		}

		const MusicName musicName = clock.nightMusicIsActive() ?
			MusicName::Night : GameData::getExteriorMusicName(weatherType);
		game.setMusic(musicName);
	}
	else
	{
		const int localDungeonID = this->travelData.locationID - 32;

		if ((localDungeonID == 0) || (localDungeonID == 1))
		{
			// Main quest dungeon.
			const auto &cityData = gameData.getCityDataFile();
			const uint32_t dungeonSeed = cityData.getDungeonSeed(
				localDungeonID, this->travelData.provinceID);
			const std::string mifName = CityDataFile::getMainQuestDungeonMifName(dungeonSeed);
			const MIFFile mif(mifName);
			const Location location = Location::makeDungeon(
				localDungeonID, this->travelData.provinceID);
			gameData.loadInterior(mif, location,
				game.getTextureManager(), game.getRenderer());
		}
		else
		{
			// Random named dungeon.
			const bool isArtifactDungeon = false;
			gameData.loadNamedDungeon(localDungeonID, this->travelData.provinceID,
				isArtifactDungeon, game.getTextureManager(), game.getRenderer());
		}

		const MusicName musicName = GameData::getDungeonMusicName(random);
		game.setMusic(musicName);
	}

	game.popSubPanel();
	game.setPanel<GameWorldPanel>(game);
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
		this->switchToGameWorld();
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
