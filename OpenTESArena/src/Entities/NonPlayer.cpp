#include "EntityType.h"
#include "NonPlayer.h"
#include "../Math/Constants.h"

NonPlayer::NonPlayer(const Double2 &position, const Double2 &direction)
	: Entity(EntityType::NonPlayer), camera(position, direction), velocity(0.0, 0.0) { }

NonPlayer::NonPlayer()
	: NonPlayer(Double2::Zero, Double2::UnitX) { }

void NonPlayer::tick(Game &game, double dt)
{
	// @todo
	DebugNotImplemented();
}
