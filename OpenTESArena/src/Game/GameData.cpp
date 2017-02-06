#include <algorithm>
#include <cassert>

#include "GameData.h"

#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelGrid.h"

GameData::GameData(std::unique_ptr<Player> player,
	std::unique_ptr<EntityManager> entityManager, 
	std::unique_ptr<VoxelGrid> voxelGrid,
	double gameTime, double viewDistance)
{
	Debug::mention("GameData", "Initializing.");

	this->player = std::move(player);
	this->entityManager = std::move(entityManager);
	this->voxelGrid = std::move(voxelGrid);

	// Initialize collision grid to empty.
	const int worldVolume = this->voxelGrid->getWidth() * this->voxelGrid->getHeight() *
		this->voxelGrid->getDepth();
	this->collisionGrid = std::vector<char>(worldVolume);
	std::fill(this->collisionGrid.begin(), this->collisionGrid.end(), 0);

	this->gameTime = gameTime;
	this->viewDistance = viewDistance;
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

VoxelGrid &GameData::getVoxelGrid()
{
	return *this->voxelGrid.get();
}

std::vector<char> &GameData::getCollisionGrid()
{
	return this->collisionGrid;
}

double GameData::getGameTime() const
{
	return this->gameTime;
}

double GameData::getViewDistance() const
{
	return this->viewDistance;
}

void GameData::incrementGameTime(double dt)
{
	assert(dt >= 0.0);

	this->gameTime += dt;
}
