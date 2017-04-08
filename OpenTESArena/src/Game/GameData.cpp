#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "GameData.h"

#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Math/Random.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelGrid.h"

const double GameData::SECONDS_PER_DAY = 120.0; // Arbitrary value for testing.

GameData::GameData(Player &&player, EntityManager &&entityManager, VoxelGrid &&voxelGrid,
	double gameTime, double fogDistance)
	: player(std::move(player)), entityManager(std::move(entityManager)),
	voxelGrid(std::move(voxelGrid))
{
	Debug::mention("GameData", "Initializing.");

	this->gameTime = gameTime;
	this->fogDistance = fogDistance;
}

GameData::~GameData()
{
	Debug::mention("GameData", "Closing.");
}

std::unique_ptr<GameData> GameData::createDefault(const std::string &playerName,
	GenderName gender, int raceID, const CharacterClass &charClass, int portraitID,
	TextureManager &textureManager, Renderer &renderer)
{
	// Create some dummy data for the test world.
	EntityManager entityManager;

	// Some arbitrary player values.
	const Double3 position = Double3(1.50, 1.70, 12.50);
	const Double3 direction = Double3(1.0, 0.0, 0.0).normalized();
	const Double3 velocity = Double3(0.0, 0.0, 0.0);
	const double maxWalkSpeed = 2.0;
	const double maxRunSpeed = 8.0;

	Player player(playerName, gender, raceID, charClass, portraitID,
		position, direction, velocity, maxWalkSpeed, maxRunSpeed);

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
		textureManager.getSurface("NBRIDGE.IMG"),
	};

	for (const auto *surface : surfaces)
	{
		renderer.addTexture(static_cast<uint32_t*>(surface->pixels),
			surface->w, surface->h);
	}

	// Make an empty voxel grid with some arbitrary dimensions.
	const int gridWidth = 24;
	const int gridHeight = 5;
	const int gridDepth = 24;
	VoxelGrid voxelGrid(gridWidth, gridHeight, gridDepth);

	// Add some voxel data for the voxel grid's IDs to refer to. The first voxel data 
	// is a placeholder for "empty voxels", so subtract 1 from the wall ID to get the
	// texture index.
	// - A wall/floor/ceiling ID of 0 indicates air. The "empty voxel" is defined
	//   as having air for each voxel face, and is ignored during rendering.
	const int emptyID = voxelGrid.addVoxelData(VoxelData(0));

	// City wall.
	const int cityWallID = voxelGrid.addVoxelData(VoxelData(1));

	// Ground (with sea wall).
	const int gravelID = voxelGrid.addVoxelData(VoxelData(2, 0, 3));
	const int roadID = voxelGrid.addVoxelData(VoxelData(2, 0, 4));
	const int grassID = voxelGrid.addVoxelData(VoxelData(2, 0, 5));

	// Tavern.
	const int tavern1ID = voxelGrid.addVoxelData(VoxelData(8));
	const int tavern2ID = voxelGrid.addVoxelData(VoxelData(9));
	const int tavern3ID = voxelGrid.addVoxelData(VoxelData(10));
	const int tavernDoorID = voxelGrid.addVoxelData(VoxelData(11));

	// Temple.
	const int temple1ID = voxelGrid.addVoxelData(VoxelData(12));
	const int temple2ID = voxelGrid.addVoxelData(VoxelData(13));
	const int temple3ID = voxelGrid.addVoxelData(VoxelData(14));
	const int temple4ID = voxelGrid.addVoxelData(VoxelData(15));
	const int temple5ID = voxelGrid.addVoxelData(VoxelData(16));
	const int templeDoorID = voxelGrid.addVoxelData(VoxelData(17));

	// Mage's guild.
	const int mages1ID = voxelGrid.addVoxelData(VoxelData(18));
	const int mages2ID = voxelGrid.addVoxelData(VoxelData(19));
	const int mages3ID = voxelGrid.addVoxelData(VoxelData(20));
	const int mages4ID = voxelGrid.addVoxelData(VoxelData(21));
	const int mages5ID = voxelGrid.addVoxelData(VoxelData(22));
	const int magesDoorID = voxelGrid.addVoxelData(VoxelData(23));

	// Equipment store.
	const int equip1ID = voxelGrid.addVoxelData(VoxelData(24));
	const int equip2ID = voxelGrid.addVoxelData(VoxelData(25));
	const int equip3ID = voxelGrid.addVoxelData(VoxelData(26));
	const int equipDoorID = voxelGrid.addVoxelData(VoxelData(27));

	// Low house.
	const int lowHouse1ID = voxelGrid.addVoxelData(VoxelData(28));
	const int lowHouse2ID = voxelGrid.addVoxelData(VoxelData(29));
	const int lowHouse3ID = voxelGrid.addVoxelData(VoxelData(30));
	const int lowHouse4ID = voxelGrid.addVoxelData(VoxelData(31));
	const int lowHouseDoorID = voxelGrid.addVoxelData(VoxelData(32));

	// Medium house.
	const int medHouse1ID = voxelGrid.addVoxelData(VoxelData(33));
	const int medHouse2ID = voxelGrid.addVoxelData(VoxelData(34));
	const int medHouse3ID = voxelGrid.addVoxelData(VoxelData(35));
	const int medHouseDoorID = voxelGrid.addVoxelData(VoxelData(36));

	// Noble house.
	const int noble1ID = voxelGrid.addVoxelData(VoxelData(37));
	const int noble2ID = voxelGrid.addVoxelData(VoxelData(38));
	const int noble3ID = voxelGrid.addVoxelData(VoxelData(39));
	const int nobleDoorID = voxelGrid.addVoxelData(VoxelData(40));

	// Hedge.
	const int hedgeID = voxelGrid.addVoxelData(VoxelData(41, 0, 0));

	// Bridge.
	const int bridge1ID = voxelGrid.addVoxelData(VoxelData(42, 43, 43, 0.0, 0.125, 0.875, 1.0));
	const int bridge2ID = voxelGrid.addVoxelData(VoxelData(42, 43, 43, 0.10, 0.125, 0.775, 0.90));

	// Lambda for setting a voxel at some coordinate to some ID.
	auto setVoxel = [&voxelGrid](int x, int y, int z, int id)
	{
		char *voxels = voxelGrid.getVoxels();
		voxels[x + (y * voxelGrid.getWidth()) +
			(z * voxelGrid.getWidth() * voxelGrid.getHeight())] = id;
	};

	// Set voxel IDs with indices into the voxel data.
	// City walls.
	for (int j = 0; j < (gridHeight - 1); ++j)
	{
		for (int k = 0; k < gridDepth; ++k)
		{
			setVoxel(0, j, k, cityWallID);
			setVoxel(gridWidth - 1, j, k, cityWallID);
		}
	}

	for (int j = 0; j < (gridHeight - 1); ++j)
	{
		for (int i = 0; i < gridWidth; ++i)
		{
			setVoxel(i, j, 0, cityWallID);
			setVoxel(i, j, gridDepth - 1, cityWallID);
		}
	}

	// Grass fill.
	for (int k = 1; k < (gridDepth - 1); ++k)
	{
		for (int i = 1; i < (gridWidth - 1); ++i)
		{
			setVoxel(i, 0, k, grassID);
		}
	}

	// Road.
	for (int i = 1; i < (gridWidth - 1); ++i)
	{
		setVoxel(i, 0, 11, roadID);
		setVoxel(i, 0, 12, roadID);
		setVoxel(i, 0, 13, roadID);
	}

	// Trench (with water eventually).
	for (int k = 1; k < (gridDepth - 1); ++k)
	{
		setVoxel(11, 0, k, emptyID);
		setVoxel(12, 0, k, emptyID);
	}

	// Random number generator with an arbitrary seed.
	Random random(0);

	// Tavern.
	for (int k = 5; k < 10; ++k)
	{
		for (int j = 1; j < 3; ++j)
		{
			for (int i = 2; i < 6; ++i)
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
	for (int k = 2; k < 10; ++k)
	{
		for (int j = 1; j < 4; ++j)
		{
			for (int i = 7; i < 10; ++i)
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
	for (int k = 15; k < 20; ++k)
	{
		for (int j = 1; j < 3; ++j)
		{
			for (int i = 7; i < 10; ++i)
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
	for (int k = 15; k < 19; ++k)
	{
		for (int j = 1; j < 2; ++j)
		{
			for (int i = 2; i < 5; ++i)
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
	for (int k = 15; k < 20; ++k)
	{
		for (int j = 1; j < 2; ++j)
		{
			for (int i = 14; i < 18; ++i)
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
	for (int k = 15; k < 19; ++k)
	{
		for (int j = 1; j < 3; ++j)
		{
			for (int i = 19; i < 22; ++i)
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
	for (int k = 4; k < 9; ++k)
	{
		for (int j = 1; j < 3; ++j)
		{
			for (int i = 16; i < 20; ++i)
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
	for (int k = 2; k < 10; ++k)
	{
		setVoxel(14, 1, k, hedgeID);
		setVoxel(21, 1, k, hedgeID);
	}

	for (int i = 15; i < 21; ++i)
	{
		setVoxel(i, 1, 2, hedgeID);
	}

	// Bridge.
	for (int k = 11; k < 14; ++k)
	{
		setVoxel(10, 1, k, bridge1ID);
		setVoxel(11, 1, k, bridge2ID);
		setVoxel(12, 1, k, bridge2ID);
		setVoxel(13, 1, k, bridge1ID);
	}

	// Add sprites.
	const SDL_Surface *tree1 = textureManager.getSurface("NPINE1.IMG");
	const SDL_Surface *tree2 = textureManager.getSurface("NPINE4.IMG");
	const SDL_Surface *statue = textureManager.getSurface("NSTATUE1.IMG");
	const SDL_Surface *lampPost = textureManager.getSurface("NLAMP1DA.IMG");
	const SDL_Surface *woman = textureManager.getSurfaces("FMGEN01.CFA").at(6);
	const SDL_Surface *man = textureManager.getSurfaces("MLGEN01W.CFA").at(6);
	const int tree1TextureID = renderer.addTexture(
		static_cast<const uint32_t*>(tree1->pixels), tree1->w, tree1->h);
	const int tree2TextureID = renderer.addTexture(
		static_cast<const uint32_t*>(tree2->pixels), tree2->w, tree2->h);
	const int statueTextureID = renderer.addTexture(
		static_cast<const uint32_t*>(statue->pixels), statue->w, statue->h);
	const int lampPostTextureID = renderer.addTexture(
		static_cast<const uint32_t*>(lampPost->pixels), lampPost->w, lampPost->h);
	const int womanTextureID = renderer.addTexture(
		static_cast<const uint32_t*>(woman->pixels), woman->w, woman->h);
	const int manTextureID = renderer.addTexture(
		static_cast<const uint32_t*>(man->pixels), man->w, man->h);
	const double tree1Scale = 2.0;
	const double tree2Scale = 2.0;
	const double statueScale = 1.0;
	const double lampPostScale = 0.90;
	const double womanScale = 0.80;
	const double manScale = 0.80;
	renderer.addFlat(
		Double3(2.50, 1.0, 21.50),
		Double2(1.0, 0.0),
		0.88 * tree1Scale,
		1.37 * tree1Scale,
		tree1TextureID);
	renderer.addFlat(
		Double3(9.50, 1.0, 21.50),
		Double2(1.0, 0.0),
		0.66 * tree2Scale,
		1.32 * tree2Scale,
		tree2TextureID);
	renderer.addFlat(
		Double3(2.50, 1.0, 2.50),
		Double2(1.0, 0.0),
		0.66 * tree2Scale,
		1.32 * tree2Scale,
		tree2TextureID);
	renderer.addFlat(
		Double3(20.50, 1.0, 21.50),
		Double2(1.0, 0.0),
		0.88 * tree1Scale,
		1.37 * tree1Scale,
		tree1TextureID);
	renderer.addFlat(
		Double3(6.50, 1.0, 12.50),
		Double2(1.0, 0.0),
		0.74 * statueScale,
		1.38 * statueScale,
		statueTextureID);
	renderer.addFlat(
		Double3(5.50, 1.0, 10.50),
		Double2(1.0, 0.0),
		0.64 * lampPostScale,
		1.03 * lampPostScale,
		lampPostTextureID);
	renderer.addFlat(
		Double3(9.50, 1.0, 14.50),
		Double2(1.0, 0.0),
		0.64 * lampPostScale,
		1.03 * lampPostScale,
		lampPostTextureID);
	renderer.addFlat(
		Double3(18.50, 1.0, 9.50),
		Double2(1.0, 0.0),
		0.64 * lampPostScale,
		1.03 * lampPostScale,
		lampPostTextureID);
	renderer.addFlat(
		Double3(17.50, 1.0, 14.50),
		Double2(1.0, 0.0),
		0.64 * lampPostScale,
		1.03 * lampPostScale,
		lampPostTextureID);
	renderer.addFlat(
		Double3(4.50, 1.0, 13.50),
		Double2(1.0, 0.0),
		0.44 * womanScale,
		1.04 * womanScale,
		womanTextureID);
	renderer.addFlat(
		Double3(4.50, 1.0, 11.50),
		Double2(1.0, 0.0),
		0.52 * manScale,
		0.99 * manScale,
		manTextureID);

	// Fog distance is changed infrequently, so it can go here in scene creation.
	// It's not an expensive operation for the software renderer.
	const double fogDistance = 18.0;
	renderer.setFogDistance(fogDistance);

	// The sky palette is used to color the sky and fog. The renderer chooses
	// which color to use based on the time of day. Interiors should just have
	// one pixel as the sky palette (usually black).
	const SDL_Surface *skyPalette = textureManager.getSurface("DAYTIME.COL");
	renderer.setSkyPalette(static_cast<const uint32_t*>(skyPalette->pixels),
		skyPalette->w * skyPalette->h);

	const double gameTime = 0.0; // In seconds. Affects time of day.

	return std::unique_ptr<GameData>(new GameData(
		std::move(player), std::move(entityManager),
		std::move(voxelGrid), gameTime, fogDistance));
}

Player &GameData::getPlayer()
{
	return this->player;
}

EntityManager &GameData::getEntityManager()
{
	return this->entityManager;
}

VoxelGrid &GameData::getVoxelGrid()
{
	return this->voxelGrid;
}

double GameData::getGameTime() const
{
	return this->gameTime;
}

double GameData::getDaytimePercent() const
{
	return std::fmod(this->getGameTime(), GameData::SECONDS_PER_DAY) /
		GameData::SECONDS_PER_DAY;
}

double GameData::getFogDistance() const
{
	return this->fogDistance;
}

void GameData::incrementGameTime(double dt)
{
	assert(dt >= 0.0);

	this->gameTime += dt;
}
