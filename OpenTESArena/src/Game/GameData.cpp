#include <cassert>

#include "GameData.h"

#include "../Entities/Player.h"
#include "../Entities/EntityManager.h"
#include "../Rendering/CLProgram.h"
#include "../Utilities/Debug.h"

GameData::GameData(std::unique_ptr<Player> player, 
	std::unique_ptr<EntityManager> entityManager,
	std::unique_ptr<CLProgram> clProgram)
{
	Debug::mention("GameData", "Initializing.");

	this->player = std::move(player);
	this->entityManager = std::move(entityManager);
	this->clProgram = std::move(clProgram);

	assert(this->player.get() != nullptr);
	assert(this->entityManager.get() != nullptr);
	assert(this->clProgram.get() != nullptr);
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

CLProgram &GameData::getCLProgram() const
{
	return *this->clProgram.get();
}
