#include <algorithm>
#include <cassert>

#include "GameData.h"

#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Utilities/Debug.h"

GameData::GameData(std::unique_ptr<Player> player,
	std::unique_ptr<EntityManager> entityManager, double gameTime, 
	int worldWidth, int worldHeight, int worldDepth)
{
	Debug::mention("GameData", "Initializing.");

	this->player = std::move(player);
	this->entityManager = std::move(entityManager);

	// Initialize 3D grids.
	const int worldVolume = worldWidth * worldHeight * worldDepth;
	this->voxelGrid = std::vector<char>(worldVolume);
	this->collisionGrid = std::vector<char>(worldVolume);
	std::fill(this->voxelGrid.begin(), this->voxelGrid.end(), 0);
	std::fill(this->collisionGrid.begin(), this->collisionGrid.end(), 0);

	this->gameTime = gameTime;
	this->worldWidth = worldWidth;
	this->worldHeight = worldHeight;
	this->worldDepth = worldDepth;
}

GameData::~GameData()
{
	Debug::mention("GameData", "Closing.");
}

Player &GameData::getPlayer() const
{
	return *this->player.get();
}

EntityManager &GameData::getEntityManager() const
{
	return *this->entityManager.get();
}

std::vector<char> &GameData::getVoxelGrid()
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

int GameData::getWorldWidth() const
{
	return this->worldWidth;
}

int GameData::getWorldHeight() const
{
	return this->worldHeight;
}

int GameData::getWorldDepth() const
{
	return this->worldDepth;
}

void GameData::incrementGameTime(double dt)
{
	assert(dt >= 0.0);

	this->gameTime += dt;
}
