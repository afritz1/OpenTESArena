#include <cassert>

#include "Player.h"

#include "../Game/GameState.h"

Player::Player(const std::string &displayName, const Float3d &position, 
	const Float3d &direction, const Float3d &velocity, EntityManager &entityManager)
	: Entity(EntityType::Player, position, entityManager), Directable(direction), 
	Movable(velocity)
{
	this->displayName = displayName;
}

Player::~Player()
{

}

std::unique_ptr<Entity> Player::clone(EntityManager &entityManager) const
{
	return std::unique_ptr<Entity>(new Player(
		this->getDisplayName(), this->getPosition(), this->getDirection(),
		this->getVelocity(), entityManager));
}

const std::string &Player::getDisplayName() const
{
	return this->displayName;
}

void Player::tick(GameState *gameState, double dt)
{

}
