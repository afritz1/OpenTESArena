#include <cassert>

#include "GameData.h"

#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Utilities/Debug.h"

GameData::GameData(std::unique_ptr<Player> player, 
	std::unique_ptr<EntityManager> entityManager,
	double gameTime, int worldWidth, int worldHeight, int worldDepth)
{
	Debug::mention("GameData", "Initializing.");

	this->player = std::move(player);
	this->entityManager = std::move(entityManager);
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
