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
class EntityDefinitionLibrary;
class EntityManager;
class ExeData;
class LevelInstance;

enum class CardinalDirectionName;

class DynamicEntity final : public Entity
{
private:
	WorldDouble2 direction;
	WorldDouble2 velocity;
	std::optional<WorldDouble2> destination;
	double secondsTillCreatureSound;
	DynamicEntityType derivedType;

	// Gets the next creature sound wait time (in seconds) from the given RNG.
	static double nextCreatureSoundWaitTime(Random &random);

	// @todo: this should probably be a property of the listener, not this entity.
	bool withinHearingDistance(const CoordDouble3 &point, double ceilingScale);

	// Attempts to get the entity's creature sound filename (if any). Returns success.
	bool tryGetCreatureSoundFilename(const EntityManager &entityManager,
		const EntityDefinitionLibrary &entityDefLibrary, std::string *outFilename) const;

	// Plays the given creature sound on the entity.
	void playCreatureSound(const std::string &soundFilename, double ceilingScale,
		AudioManager &audioManager);

	// Helper method for rotating.
	void yaw(double radians);

	// Update functions for various dynamic entity types.
	void updateCitizenState(Game &game, double dt);
	void updateCreatureState(Game &game, double dt);
	void updateProjectileState(Game &game, double dt);

	// Updates the entity's physics in the world (if any).
	void updatePhysics(const LevelInstance &activeLevel, const EntityDefinitionLibrary &entityDefLibrary,
		Random &random, double dt);
public:
	DynamicEntity();
	~DynamicEntity() override = default;

	void initCitizen(EntityDefID defID, const EntityAnimationInstance &animInst,
		CardinalDirectionName direction);
	void initCreature(EntityDefID defID, const EntityAnimationInstance &animInst,
		const WorldDouble2 &direction, Random &random);
	void initProjectile(EntityDefID defID, const EntityAnimationInstance &animInst,
		const WorldDouble2 &direction);

	EntityType getEntityType() const override;
	DynamicEntityType getDerivedType() const;
	const WorldDouble2 &getDirection() const;
	const WorldDouble2 &getVelocity() const;
	const WorldDouble2 *getDestination() const;

	void setDirection(const WorldDouble2 &direction);

	// Turns the camera around the global up vector by the given degrees.
	void rotate(double degrees);

	// Faces the entity toward the given point.
	void lookAt(const CoordDouble2 &point);

	// Sets the entity's pathfinding destination and they will attempt to move towards
	// it each frame depending on their AI. If the given point is null, their destination
	// is reset.
	void setDestination(const WorldDouble2 *point, double minDistance);
	void setDestination(const WorldDouble2 *point);

	void reset() override;
	void tick(Game &game, double dt) override;
};

#endif
