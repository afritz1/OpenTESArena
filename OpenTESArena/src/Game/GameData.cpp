#include <algorithm>
#include <cassert>

#include "GameData.h"

#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Math/Random.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelGrid.h"

GameData::GameData(Player &&player, EntityManager &&entityManager, VoxelGrid &&voxelGrid,
	double gameTime, double fogDistance)
	: player(std::move(player)), entityManager(std::move(entityManager)),
	voxelGrid(std::move(voxelGrid))
{
	Debug::mention("GameData", "Initializing.");

	// Initialize collision grid to empty.
	const int worldVolume = this->voxelGrid.getWidth() * this->voxelGrid.getHeight() *
		this->voxelGrid.getDepth();
	this->collisionGrid = std::vector<char>(worldVolume);
	std::fill(this->collisionGrid.begin(), this->collisionGrid.end(), 0);

	this->gameTime = gameTime;
	this->fogDistance = fogDistance;
}

GameData::~GameData()
{
	Debug::mention("GameData", "Closing.");
}

std::unique_ptr<GameData> GameData::createDefault(const std::string &playerName,
	GenderName gender, CharacterRaceName raceName, const CharacterClass &charClass,
	int portraitID)
{
	// Create some dummy data for the test world.
	EntityManager entityManager;

	// Some arbitrary player values.
	const Double3 position = Double3(1.50, 1.70, 2.50);
	const Double3 direction = Double3(1.0, 0.0, 1.0).normalized();
	const Double3 velocity = Double3(0.0, 0.0, 0.0);
	const double maxWalkSpeed = 2.0;
	const double maxRunSpeed = 8.0;

	Player player(playerName, gender, raceName, charClass, portraitID, 
		position, direction, velocity, maxWalkSpeed, maxRunSpeed, entityManager);

	// Voxel grid with some arbitrary dimensions.
	const int gridWidth = 32;
	const int gridHeight = 5;
	const int gridDepth = 32;
	const double voxelHeight = 1.0;
	VoxelGrid voxelGrid(gridWidth, gridHeight, gridDepth, voxelHeight);

	// Add some voxel data for the voxel IDs to refer to.
	voxelGrid.addVoxelData(VoxelData(0, 0));
	voxelGrid.addVoxelData(VoxelData(1, 1));
	voxelGrid.addVoxelData(VoxelData(2, 2));

	char *voxels = voxelGrid.getVoxels();
	Random random(0);

	// Place some voxels around.
	for (int k = 0; k < gridDepth; ++k)
	{
		for (int i = 0; i < gridWidth; ++i)
		{
			// Ground.
			const int j = 0;
			const int index = i + (j * gridWidth) + (k * gridWidth * gridHeight);
			voxels[index] = 1 + random.next(3);
		}
	}

	for (int n = 0; n < 200; ++n)
	{
		const int x = random.next(gridWidth);
		const int y = 1 + random.next(gridHeight - 1);
		const int z = random.next(gridDepth);

		const int index = x + (y * gridWidth) + (z * gridWidth * gridHeight);
		voxels[index] = 1 + random.next(3);
	}

	const double gameTime = 0.0; // In seconds. Affects time of day.
	const double fogDistance = 15.0;

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

std::vector<char> &GameData::getCollisionGrid()
{
	return this->collisionGrid;
}

double GameData::getGameTime() const
{
	return this->gameTime;
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
