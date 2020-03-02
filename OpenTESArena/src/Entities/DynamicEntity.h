#ifndef DYNAMIC_ENTITY_H
#define DYNAMIC_ENTITY_H

#include <optional>
#include <string>

#include "DynamicEntityType.h"
#include "Entity.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// An entity that can move and look in different directions. The displayed texture depends
// on the entity's position relative to the player's camera.

class AudioManager;
class EntityManager;
class ExeData;
class WorldData;

class DynamicEntity final : public Entity
{
private:
	Double2 direction;
	Double2 velocity;
	std::optional<Double2> destination;
	double secondsTillCreatureSound;
	DynamicEntityType derivedType;

	// Gets the next creature sound wait time (in seconds) from the given RNG.
	static double nextCreatureSoundWaitTime(Random &random);

	// @todo: this should probably be a property of the listener, not this entity.
	bool withinHearingDistance(const Double3 &point, double ceilingHeight);

	// Attempts to get the entity's creature sound filename (if any). Returns success.
	bool tryGetCreatureSoundFilename(const EntityManager &entityManager,
		const ExeData &exeData, std::string *outFilename) const;

	// Plays the given creature sound on the entity.
	void playCreatureSound(const std::string &soundFilename, double ceilingHeight,
		AudioManager &audioManager);

	// Helper method for rotating.
	void yaw(double radians);

	// Updates NPC-relevant data in the entity.
	void updateNpcState(Game &game, double dt);

	// Updates the entity's physics in the world (if any).
	void updatePhysics(const WorldData &worldData, double dt);
public:
	DynamicEntity();
	virtual ~DynamicEntity() = default;

	EntityType getEntityType() const override;
	DynamicEntityType getDerivedType() const;
	const Double2 &getDirection() const;
	const Double2 &getVelocity() const;
	const Double2 *getDestination() const;

	void setDerivedType(DynamicEntityType derivedType);
	void setDirection(const Double2 &direction);

	// Turns the camera around the global up vector by the given degrees.
	void rotate(double degrees);

	// Faces the entity toward the given point.
	void lookAt(const Double2 &point);

	// Sets the entity's pathfinding destination and they will attempt to move towards
	// it each frame depending on their AI. If the given point is null, their destination
	// is reset.
	void setDestination(const Double2 *point, double minDistance);
	void setDestination(const Double2 *point);

	virtual void reset() override;
	virtual void tick(Game &game, double dt) override;
};

#endif
