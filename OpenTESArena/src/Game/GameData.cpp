#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "GameData.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Entities/Animation.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Doodad.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityManager.h"
#include "../Entities/GenderName.h"
#include "../Entities/NonPlayer.h"
#include "../Entities/Player.h"
#include "../Interface/TextBox.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/ClimateType.h"
#include "../World/LocationType.h"
#include "../World/VoxelGrid.h"
#include "../World/VoxelType.h"
#include "../World/WeatherType.h"
#include "../World/WorldType.h"

// Arbitrary value for testing. One real second = six game minutes.
// The value used in Arena is one real second = twenty game seconds.
const double GameData::TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 240.0;

GameData::GameData(Player &&player, WorldData &&worldData, const Location &location,
	const Date &date, const Clock &clock, double fogDistance)
	: player(std::move(player)), worldData(std::move(worldData)), location(location),
	date(date), clock(clock), triggerText(0.0, nullptr), actionText(0.0, nullptr),
	effectText(0.0, nullptr)
{
	DebugMention("Initializing.");

	this->fogDistance = fogDistance;
}

GameData::~GameData()
{
	DebugMention("Closing.");
}

std::vector<uint32_t> GameData::makeExteriorSkyPalette(WeatherType weatherType,
	TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const std::string paletteName = (weatherType == WeatherType::Clear) ?
		"DAYTIME.COL" : "DREARY.COL";

	// The palettes in the data files only cover half of the day, so some added
	// darkness is needed for the other half.
	const SDL_Surface *palette = textureManager.getSurface(paletteName);
	const uint32_t *pixels = static_cast<const uint32_t*>(palette->pixels);
	const int pixelCount = palette->w * palette->h;

	std::vector<uint32_t> fullPalette(pixelCount * 2);

	// Fill with darkness (the first color in the palette is the closest to night).
	const uint32_t darkness = pixels[0];
	std::fill(fullPalette.begin(), fullPalette.end(), darkness);

	// Copy the sky palette over the center of the full palette.
	std::copy(pixels, pixels + pixelCount, 
		fullPalette.data() + (fullPalette.size() / 4));

	return fullPalette;
}

double GameData::getFogDistanceFromWeather(WeatherType weatherType)
{
	// Arbitrary values, the distance at which fog is maximum.
	if (weatherType == WeatherType::Clear)
	{
		return 75.0;
	}
	else if (weatherType == WeatherType::Overcast)
	{
		return 25.0;
	}
	else if (weatherType == WeatherType::Rain)
	{
		return 35.0;
	}
	else if (weatherType == WeatherType::Snow)
	{
		return 15.0;
	}
	else
	{
		throw std::runtime_error("Bad weather type \"" +
			std::to_string(static_cast<int>(weatherType)) + "\".");
	}
}

void GameData::loadInterior(const MIFFile &mif, Double3 &playerPosition, WorldData &worldData,
	TextureManager &textureManager, Renderer &renderer)
{
	// Call interior WorldData loader.
	worldData = WorldData::loadInterior(mif);
	worldData.setLevelActive(worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = worldData.getStartPoints().front();
	playerPosition = Double3(startPoint.x, playerPosition.y, startPoint.y);

	// Set interior sky palette.
	const auto &level = worldData.getLevels().at(worldData.getCurrentLevel());
	const uint32_t skyColor = level.getInteriorSkyColor();
	renderer.setSkyPalette(&skyColor, 1);

	// Arbitrary interior fog distance.
	renderer.setFogDistance(25.0);
}

void GameData::loadPremadeCity(const MIFFile &mif, ClimateType climateType,
	WeatherType weatherType, Double3 &playerPosition, WorldData &worldData,
	TextureManager &textureManager, Renderer &renderer)
{
	// Call premade WorldData loader.
	worldData = WorldData::loadPremadeCity(mif, climateType, weatherType);
	worldData.setLevelActive(worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = worldData.getStartPoints().front();
	playerPosition = Double3(startPoint.x, playerPosition.y, startPoint.y);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));
	
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	renderer.setFogDistance(fogDistance);
}

void GameData::loadCity(const MIFFile &mif, WeatherType weatherType, Double3 &playerPosition,
	WorldData &worldData, TextureManager &textureManager, Renderer &renderer)
{
	// Call random city WorldData loader.
	worldData = WorldData::loadCity(mif, weatherType);
	worldData.setLevelActive(worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = worldData.getStartPoints().front();
	playerPosition = Double3(startPoint.x, playerPosition.y, startPoint.y);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	renderer.setFogDistance(fogDistance);
}

void GameData::loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL, ClimateType climateType,
	WeatherType weatherType, Double3 &playerPosition, WorldData &worldData,
	TextureManager &textureManager, Renderer &renderer)
{
	// Call wilderness WorldData loader.
	worldData = WorldData::loadWilderness(rmdTR, rmdTL, rmdBR, rmdBL, climateType, weatherType);
	worldData.setLevelActive(worldData.getCurrentLevel(), textureManager, renderer);

	// Set arbitrary player starting position (no starting point in WILD.MIF).
	const Double2 startPoint(63.50, 63.50);
	playerPosition = Double3(startPoint.x, playerPosition.y, startPoint.y);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	renderer.setFogDistance(fogDistance);
}

std::unique_ptr<GameData> GameData::createDefault(const std::string &playerName,
	GenderName gender, int raceID, const CharacterClass &charClass, int portraitID,
	const ExeStrings &exeStrings, TextureManager &textureManager, Renderer &renderer)
{
	// Create some dummy data for the test world.

	// Some arbitrary player values.
	const Double3 position = Double3(1.50, 1.70, 12.50);
	const Double3 direction = Double3(1.0, 0.0, 0.0).normalized();
	const Double3 velocity = Double3::Zero;
	const double maxWalkSpeed = 2.0;
	const double maxRunSpeed = 8.0;
	const int weaponID = [&charClass]()
	{
		// Pick a random weapon available to the player's class for testing.
		std::vector<int> weapons = charClass.getAllowedWeapons();

		// Remove bows for now (16 and 17).
		weapons.erase(std::remove(weapons.begin(), weapons.end(), 16), weapons.end());
		weapons.erase(std::remove(weapons.begin(), weapons.end(), 17), weapons.end());

		// Add fists.
		weapons.push_back(-1);

		Random random;
		const int index = random.next(static_cast<int>(weapons.size()));
		return weapons.at(index);
	}();

	Player player(playerName, gender, raceID, charClass, portraitID, position,
		direction, velocity, maxWalkSpeed, maxRunSpeed, weaponID, exeStrings);

	// Add some wall textures.
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));
	std::vector<const SDL_Surface*> surfaces = {
		// Texture indices:
		// 0: city wall
		textureManager.getSurface("CITYWALL.IMG"),

		// 1: sea wall
		textureManager.getSurface("SEAWALL.IMG"),

		// 2-4: grounds
		textureManager.getSurfaces("NORM1.SET").at(0),
		textureManager.getSurfaces("NORM1.SET").at(1),
		textureManager.getSurfaces("NORM1.SET").at(2),

		// 5-6: gates
		textureManager.getSurface("DLGT.IMG"),
		textureManager.getSurface("DRGT.IMG"),

		// 7-10: tavern + door
		textureManager.getSurfaces("MTAVERN.SET").at(0),
		textureManager.getSurfaces("MTAVERN.SET").at(1),
		textureManager.getSurfaces("MTAVERN.SET").at(2),
		textureManager.getSurface("DTAV.IMG"),

		// 11-16: temple + door
		textureManager.getSurfaces("MTEMPLE.SET").at(0),
		textureManager.getSurfaces("MTEMPLE.SET").at(1),
		textureManager.getSurfaces("MTEMPLE.SET").at(2),
		textureManager.getSurfaces("MTEMPLE.SET").at(3),
		textureManager.getSurfaces("MTEMPLE.SET").at(4),
		textureManager.getSurface("DTEP.IMG"),

		// 17-22: Mage's Guild + door
		textureManager.getSurfaces("MMUGUILD.SET").at(0),
		textureManager.getSurfaces("MMUGUILD.SET").at(1),
		textureManager.getSurfaces("MMUGUILD.SET").at(2),
		textureManager.getSurfaces("MMUGUILD.SET").at(3),
		textureManager.getSurfaces("MMUGUILD.SET").at(4),
		textureManager.getSurface("DMU.IMG"),

		// 23-26: Equipment store + door
		textureManager.getSurfaces("MEQUIP.SET").at(0),
		textureManager.getSurfaces("MEQUIP.SET").at(1),
		textureManager.getSurfaces("MEQUIP.SET").at(2),
		textureManager.getSurface("DEQ.IMG"),

		// 27-31: Low house + door
		textureManager.getSurfaces("MBS1.SET").at(0),
		textureManager.getSurfaces("MBS1.SET").at(1),
		textureManager.getSurfaces("MBS1.SET").at(2),
		textureManager.getSurfaces("MBS1.SET").at(3),
		textureManager.getSurface("DBS1.IMG"),

		// 32-35: Medium house + door
		textureManager.getSurfaces("MBS3.SET").at(0),
		textureManager.getSurfaces("MBS3.SET").at(1),
		textureManager.getSurfaces("MBS3.SET").at(2),
		textureManager.getSurface("DBS3.IMG"),

		// 36-39: Noble house + door
		textureManager.getSurfaces("MNOBLE.SET").at(0),
		textureManager.getSurfaces("MNOBLE.SET").at(1),
		textureManager.getSurfaces("MNOBLE.SET").at(2),
		textureManager.getSurface("DNB1.IMG"),

		// 40: Hedge
		textureManager.getSurface("HEDGE.IMG"),

		// 41-42: Bridge
		textureManager.getSurface("TTOWER.IMG"),
		textureManager.getSurface("NBRIDGE.IMG")
	};

	const int wallTextureCount = static_cast<int>(surfaces.size());
	for (int i = 0; i < wallTextureCount; i++)
	{
		const SDL_Surface *surface = surfaces.at(i);
		renderer.setVoxelTexture(i, static_cast<uint32_t*>(surface->pixels));
	}

	// Make an empty voxel grid with some arbitrary dimensions.
	const int gridWidth = 24;
	const int gridHeight = 5;
	const int gridDepth = 24;
	VoxelGrid voxelGrid(gridWidth, gridHeight, gridDepth);

	// Add some voxel data for the voxel grid's IDs to refer to. The first voxel data 
	// is a placeholder for empty voxels.
	const int emptyID = voxelGrid.addVoxelData(VoxelData());

	auto addWall = [&voxelGrid](int id)
	{
		return voxelGrid.addVoxelData(
			VoxelData::makeWall(id, id, id, VoxelType::Solid));
	};

	// City wall.
	const int cityWallID = addWall(0);

	// Ground.
	const int gravelID = voxelGrid.addVoxelData(VoxelData::makeFloor(2));
	const int roadID = voxelGrid.addVoxelData(VoxelData::makeFloor(3));
	const int grassID = voxelGrid.addVoxelData(VoxelData::makeFloor(4));

	// Tavern.
	const int tavern1ID = addWall(7);
	const int tavern2ID = addWall(8);
	const int tavern3ID = addWall(9);
	const int tavernDoorID = addWall(10);

	// Temple.
	const int temple1ID = addWall(11);
	const int temple2ID = addWall(12);
	const int temple3ID = addWall(13);
	const int temple4ID = addWall(14);
	const int temple5ID = addWall(15);
	const int templeDoorID = addWall(16);

	// Mage's guild.
	const int mages1ID = addWall(17);
	const int mages2ID = addWall(18);
	const int mages3ID = addWall(19);
	const int mages4ID = addWall(20);
	const int mages5ID = addWall(21);
	const int magesDoorID = addWall(22);

	// Equipment store.
	const int equip1ID = addWall(23);
	const int equip2ID = addWall(24);
	const int equip3ID = addWall(25);
	const int equipDoorID = addWall(26);

	// Low house.
	const int lowHouse1ID = addWall(27);
	const int lowHouse2ID = addWall(28);
	const int lowHouse3ID = addWall(29);
	const int lowHouse4ID = addWall(30);
	const int lowHouseDoorID = addWall(31);

	// Medium house.
	const int medHouse1ID = addWall(32);
	const int medHouse2ID = addWall(33);
	const int medHouse3ID = addWall(34);
	const int medHouseDoorID = addWall(35);

	// Noble house.
	const int noble1ID = addWall(36);
	const int noble2ID = addWall(37);
	const int noble3ID = addWall(38);
	const int nobleDoorID = addWall(39);

	// Hedge.
	const bool hedgeIsCollider = true;
	const int hedgeID = voxelGrid.addVoxelData(
		VoxelData::makeTransparentWall(40, hedgeIsCollider));

	// Bridge.
	const int bridge1ID = voxelGrid.addVoxelData(VoxelData::makeRaised(
		41, 42, 42, 0.0, 0.125, 0.875, Constants::JustBelowOne));
	const int bridge2ID = voxelGrid.addVoxelData(VoxelData::makeRaised(
		41, 42, 42, 0.10, 0.125, 0.775, 0.90));

	// Water.
	const int water1ID = voxelGrid.addVoxelData(VoxelData::makeChasm(
		1, false, false, true, false, VoxelData::ChasmData::Type::Wet));
	const int water2ID = voxelGrid.addVoxelData(VoxelData::makeChasm(
		1, true, false, false, false, VoxelData::ChasmData::Type::Wet));

	// Lambda for setting a voxel at some coordinate to some ID.
	auto setVoxel = [&voxelGrid](int x, int y, int z, int id)
	{
		uint8_t *voxels = voxelGrid.getVoxels();
		voxels[x + (y * voxelGrid.getWidth()) +
			(z * voxelGrid.getWidth() * voxelGrid.getHeight())] = id;
	};

	// Set voxel IDs with indices into the voxel data.
	// City walls.
	for (int j = 0; j < (gridHeight - 1); j++)
	{
		for (int k = 0; k < gridDepth; k++)
		{
			setVoxel(0, j, k, cityWallID);
			setVoxel(gridWidth - 1, j, k, cityWallID);
		}
	}

	for (int j = 0; j < (gridHeight - 1); j++)
	{
		for (int i = 0; i < gridWidth; i++)
		{
			setVoxel(i, j, 0, cityWallID);
			setVoxel(i, j, gridDepth - 1, cityWallID);
		}
	}

	// Grass fill.
	for (int k = 1; k < (gridDepth - 1); k++)
	{
		for (int i = 1; i < (gridWidth - 1); i++)
		{
			setVoxel(i, 0, k, grassID);
		}
	}

	// Road.
	for (int i = 1; i < (gridWidth - 1); i++)
	{
		setVoxel(i, 0, 11, roadID);
		setVoxel(i, 0, 12, roadID);
		setVoxel(i, 0, 13, roadID);
	}

	// Trench.
	for (int k = 1; k < (gridDepth - 1); k++)
	{
		setVoxel(11, 0, k, water1ID);
		setVoxel(12, 0, k, water2ID);
	}

	// Random number generator with an arbitrary seed.
	Random random(0);

	// Tavern.
	for (int k = 5; k < 10; k++)
	{
		for (int j = 1; j < 3; j++)
		{
			for (int i = 2; i < 6; i++)
			{
				setVoxel(i, j, k, tavern1ID + random.next(3));
			}
		}
	}

	// Tavern door.
	setVoxel(3, 1, 9, tavernDoorID);

	// Tavern gravel.
	setVoxel(3, 0, 10, gravelID);

	// Temple.
	for (int k = 2; k < 10; k++)
	{
		for (int j = 1; j < 4; j++)
		{
			for (int i = 7; i < 10; i++)
			{
				setVoxel(i, j, k, temple1ID + random.next(5));
			}
		}
	}

	// Temple door.
	setVoxel(8, 1, 9, templeDoorID);

	// Temple gravel.
	setVoxel(8, 0, 10, gravelID);

	// Mages' guild.
	for (int k = 15; k < 20; k++)
	{
		for (int j = 1; j < 3; j++)
		{
			for (int i = 7; i < 10; i++)
			{
				setVoxel(i, j, k, mages1ID + random.next(5));
			}
		}
	}

	// Mages' guild door.
	setVoxel(8, 1, 15, magesDoorID);

	// Mages' guild gravel.
	setVoxel(8, 0, 14, gravelID);

	// Equipment store.
	for (int k = 15; k < 19; k++)
	{
		for (int j = 1; j < 2; j++)
		{
			for (int i = 2; i < 5; i++)
			{
				setVoxel(i, j, k, equip1ID + random.next(3));
			}
		}
	}

	// Equipment store door.
	setVoxel(3, 1, 15, equipDoorID);

	// Equipment store gravel.
	setVoxel(3, 0, 14, gravelID);

	// Low house.
	for (int k = 15; k < 20; k++)
	{
		for (int j = 1; j < 2; j++)
		{
			for (int i = 14; i < 18; i++)
			{
				setVoxel(i, j, k, lowHouse1ID + random.next(4));
			}
		}
	}

	// Low house door.
	setVoxel(15, 1, 15, lowHouseDoorID);

	// Low house gravel.
	setVoxel(15, 0, 14, gravelID);

	// Medium house.
	for (int k = 15; k < 19; k++)
	{
		for (int j = 1; j < 3; j++)
		{
			for (int i = 19; i < 22; i++)
			{
				setVoxel(i, j, k, medHouse1ID + random.next(3));
			}
		}
	}

	// Medium house door.
	setVoxel(20, 1, 15, medHouseDoorID);

	// Medium house gravel.
	setVoxel(20, 0, 14, gravelID);

	// Noble house.
	for (int k = 4; k < 9; k++)
	{
		for (int j = 1; j < 3; j++)
		{
			for (int i = 16; i < 20; i++)
			{
				setVoxel(i, j, k, noble1ID + random.next(3));
			}
		}
	}

	// Noble house door.
	setVoxel(17, 1, 8, nobleDoorID);

	// Noble house gravel.
	setVoxel(17, 0, 9, gravelID);
	setVoxel(17, 0, 10, gravelID);

	// Noble house hedges.
	for (int k = 2; k < 10; k++)
	{
		setVoxel(14, 1, k, hedgeID);
		setVoxel(21, 1, k, hedgeID);
	}

	for (int i = 15; i < 21; i++)
	{
		setVoxel(i, 1, 2, hedgeID);
	}

	// Bridge.
	for (int k = 11; k < 14; k++)
	{
		setVoxel(10, 1, k, bridge1ID);
		setVoxel(11, 1, k, bridge2ID);
		setVoxel(12, 1, k, bridge2ID);
		setVoxel(13, 1, k, bridge1ID);
	}

	// Lambdas for adding a new flat texture to the renderer.
	auto setFlatTexture = [&textureManager, &renderer](int id, const std::string &filename)
	{
		const SDL_Surface *surface = textureManager.getSurface(filename);
		renderer.setFlatTexture(id, static_cast<const uint32_t*>(surface->pixels),
			surface->w, surface->h);
	};

	auto setFlatTextures = [&textureManager, &renderer](int startID, const std::string &filename)
	{
		const std::vector<SDL_Surface*> &surfaces = textureManager.getSurfaces(filename);

		std::vector<int> textureIDs;		
		const int surfaceCount = static_cast<int>(surfaces.size());
		for (int i = 0; i < surfaceCount; i++)
		{
			const SDL_Surface *surface = surfaces.at(i);
			const int textureID = startID + i;
			renderer.setFlatTexture(textureID, static_cast<const uint32_t*>(surface->pixels),
				surface->w, surface->h);
			textureIDs.push_back(textureID);
		}

		return textureIDs;
	};

	// Flat texture properties. Also add them to the renderer.
	const int tree1TextureID = 0;
	const int tree2TextureID = 1;
	const int statueTextureID = 2;

	setFlatTexture(tree1TextureID, "NPINE1.IMG");
	setFlatTexture(tree2TextureID, "NPINE4.IMG");
	setFlatTexture(statueTextureID, "NSTATUE1.IMG");
	const std::vector<int> lampPostTextureIDs = setFlatTextures(
		statueTextureID + 1, "NLAMP1.DFA");
	const std::vector<int> womanTextureIDs = setFlatTextures(
		lampPostTextureIDs.back() + 1, "FMGEN01.CFA");
	const std::vector<int> manTextureIDs = setFlatTextures(
		womanTextureIDs.back() + 1, "MLGEN01W.CFA");

	const double tree1Scale = 2.0;
	const double tree2Scale = 2.0;
	const double statueScale = 1.0;
	const double lampPostScale = 0.90;
	const double womanScale = 0.80;
	const double manScale = 0.80;

	// Lambdas for adding entities to the entity manager and renderer (they can have
	// more parameters in the future as entities grow more complex).
	EntityManager entityManager;

	auto addDoodad = [&entityManager, &renderer](const Double3 &position, double width,
		double height, const std::vector<int> &textureIDs)
	{
		const double timePerFrame = 0.10;
		Animation animation(textureIDs, timePerFrame, true);

		std::unique_ptr<Doodad> doodad(new Doodad(animation, position, entityManager));

		// Assign the entity ID with the first texture.
		renderer.addFlat(doodad->getID(), position, width, height, textureIDs.at(0));

		entityManager.add(std::move(doodad));
	};

	auto addNonPlayer = [&entityManager, &renderer](const Double3 &position,
		const Double2 &direction, double width, double height,
		const std::vector<int> &idleIDs, const std::vector<int> &moveIDs,
		const std::vector<int> &attackIDs, const std::vector<int> &deathIDs)
	{
		// Eventually, "idleIDs" and "moveIDs" should be vector<vector<int>>.
		std::vector<Animation> idleAnimations, moveAnimations;

		const double timePerFrame = 0.33;
		idleAnimations.push_back(Animation(idleIDs, timePerFrame, true));
		moveAnimations.push_back(Animation(moveIDs, timePerFrame, true));

		Animation attackAnimation(attackIDs, timePerFrame, false);
		Animation deathAnimation(deathIDs, timePerFrame, false);

		std::unique_ptr<NonPlayer> nonPlayer(new NonPlayer(
			position, direction, idleAnimations, moveAnimations, attackAnimation,
			deathAnimation, entityManager));

		// Assign the entity ID with the first texture.
		renderer.addFlat(nonPlayer->getID(), position, width, height, idleIDs.at(0));

		entityManager.add(std::move(nonPlayer));
	};

	// Add entities.
	addDoodad(Double3(2.50, 1.0, 21.50), 0.88 * tree1Scale, 1.37 * tree1Scale, { tree1TextureID });
	addDoodad(Double3(9.50, 1.0, 21.50), 0.66 * tree2Scale, 1.32 * tree2Scale, { tree2TextureID });
	addDoodad(Double3(2.50, 1.0, 2.50), 0.66 * tree2Scale, 1.32 * tree2Scale, { tree2TextureID });
	addDoodad(Double3(20.50, 1.0, 21.50), 0.88 * tree1Scale, 1.37 * tree1Scale, { tree1TextureID });
	addDoodad(Double3(6.50, 1.0, 12.50), 0.74 * statueScale, 1.38 * statueScale, { statueTextureID });
	addDoodad(Double3(5.50, 1.0, 10.50), 0.64 * lampPostScale, 1.03 * lampPostScale, lampPostTextureIDs);
	addDoodad(Double3(9.50, 1.0, 14.50), 0.64 * lampPostScale, 1.03 * lampPostScale, lampPostTextureIDs);
	addDoodad(Double3(18.50, 1.0, 9.50), 0.64 * lampPostScale, 1.03 * lampPostScale, lampPostTextureIDs);
	addDoodad(Double3(17.50, 1.0, 14.50), 0.64 * lampPostScale, 1.03 * lampPostScale, lampPostTextureIDs);

	addNonPlayer(Double3(4.50, 1.0, 13.50), Double2(1.0, 0.0),
		0.44 * womanScale, 1.04 * womanScale, womanTextureIDs, womanTextureIDs, {}, {});
	addNonPlayer(Double3(4.50, 1.0, 11.50), Double2(1.0, 0.0),
		0.52 * manScale, 0.99 * manScale, manTextureIDs, manTextureIDs, {}, {});

	// Fog distance is changed infrequently, so it can go here in scene creation.
	// It's not an expensive operation for the software renderer.
	const double fogDistance = 18.0;
	renderer.setFogDistance(fogDistance);

	// The sky palette is used to color the sky and fog. The renderer chooses
	// which color to use based on the time of day. Interiors should just have
	// one pixel as the sky palette (usually black).
	const std::vector<uint32_t> fullSkyPalette = 
		GameData::makeExteriorSkyPalette(WeatherType::Clear, textureManager);

	renderer.setSkyPalette(fullSkyPalette.data(), static_cast<int>(fullSkyPalette.size()));

	Location location("Test City", player.getRaceID(),
		LocationType::CityState, ClimateType::Mountain);

	// Start the date on the first day of the first month.
	const int month = 0;
	const int day = 0;
	Date date(month, day);

	// Start the clock at 5:00am.
	Clock clock(5, 0, 0);
	renderer.setNightLightsActive(true);

	return std::unique_ptr<GameData>(new GameData(
		std::move(player), WorldData(std::move(voxelGrid), std::move(entityManager)),
		location, date, clock, fogDistance));
}

std::unique_ptr<GameData> GameData::createRandomPlayer(const std::vector<CharacterClass> &charClasses, 
	const ExeStrings &exeStrings, TextureManager &textureManager, Renderer &renderer)
{
	Random random;
	const std::string playerName = "Player";
	const GenderName gender = (random.next(2) == 0) ? GenderName::Male : GenderName::Female;
	const int raceID = random.next(8);
	const CharacterClass &charClass = charClasses.at(
		random.next() % static_cast<int>(charClasses.size()));
	const int portraitID = random.next(10);

	return GameData::createDefault(playerName, gender, raceID, charClass,
		portraitID, exeStrings, textureManager, renderer);
}

std::pair<double, std::unique_ptr<TextBox>> &GameData::getTriggerText()
{
	return this->triggerText;
}

std::pair<double, std::unique_ptr<TextBox>> &GameData::getActionText()
{
	return this->actionText;
}

std::pair<double, std::unique_ptr<TextBox>> &GameData::getEffectText()
{
	return this->effectText;
}

Player &GameData::getPlayer()
{
	return this->player;
}

WorldData &GameData::getWorldData()
{
	return this->worldData;
}

Location &GameData::getLocation()
{
	return this->location;
}

const Date &GameData::getDate() const
{
	return this->date;
}

const Clock &GameData::getClock() const
{
	return this->clock;
}

double GameData::getDaytimePercent() const
{
	return this->clock.getPreciseTotalSeconds() /
		static_cast<double>(Clock::SECONDS_IN_A_DAY);
}

double GameData::getFogDistance() const
{
	return this->fogDistance;
}

double GameData::getAmbientPercent() const
{
	if (this->worldData.getWorldType() == WorldType::Interior)
	{
		// Completely dark indoors (some places might be an exception to this, and those
		// would be handled eventually).
		return 0.0;
	}
	else
	{
		// The ambient light outside depends on the clock time.
		const double clockPreciseSeconds = this->clock.getPreciseTotalSeconds();

		// Time ranges where the ambient light changes. The start times are inclusive,
		// and the end times are exclusive.
		const double startBrighteningTime =
			Clock::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime =
			Clock::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime =
			Clock::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime =
			Clock::AmbientEndDimming.getPreciseTotalSeconds();

		// In Arena, the min ambient is 0 and the max ambient is 1, but we're using
		// some values here that make testing easier.
		const double minAmbient = 0.30;
		const double maxAmbient = 1.0;

		if ((clockPreciseSeconds >= endBrighteningTime) &&
			(clockPreciseSeconds < startDimmingTime))
		{
			// Daytime ambient.
			return maxAmbient;
		}
		else if ((clockPreciseSeconds >= startBrighteningTime) &&
			(clockPreciseSeconds < endBrighteningTime))
		{
			// Interpolate brightening light (in the morning).
			const double timePercent = (clockPreciseSeconds - startBrighteningTime) /
				(endBrighteningTime - startBrighteningTime);
			return minAmbient + ((maxAmbient - minAmbient) * timePercent);
		}
		else if ((clockPreciseSeconds >= startDimmingTime) &&
			(clockPreciseSeconds < endDimmingTime))
		{
			// Interpolate dimming light (in the evening).
			const double timePercent = (clockPreciseSeconds - startDimmingTime) /
				(endDimmingTime - startDimmingTime);
			return maxAmbient + ((minAmbient - maxAmbient) * timePercent);
		}
		else
		{
			// Night ambient.
			return minAmbient;
		}
	}
}

double GameData::getBetterAmbientPercent() const
{
	const double daytimePercent = this->getDaytimePercent();
	const double minAmbient = 0.20;
	const double maxAmbient = 0.90;
	const double diff = maxAmbient - minAmbient;
	const double center = minAmbient + (diff / 2.0);
	return center + ((diff / 2.0) * -std::cos(daytimePercent * (2.0 * Constants::Pi)));
}

void GameData::tickTime(double dt)
{
	assert(dt >= 0.0);

	// Tick the game clock.
	const int oldHour = this->clock.getHours24();
	this->clock.tick(dt * GameData::TIME_SCALE);
	const int newHour = this->clock.getHours24();

	// Check if the clock hour looped back around.
	if (newHour < oldHour)
	{
		// Increment the day.
		this->date.incrementDay();
	}
}
