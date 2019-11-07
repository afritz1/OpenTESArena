#ifndef DYNAMIC_ENTITY_H
#define DYNAMIC_ENTITY_H

#include <optional>

#include "DynamicEntityType.h"
#include "Entity.h"
#include "../Math/Vector2.h"

// An entity that can move and look in different directions. The displayed texture depends
// on the entity's position relative to the player's camera.

class WorldData;

class DynamicEntity final : public Entity
{
private:
	Double2 direction;
	Double2 velocity;
	std::optional<Double2> destination;
	DynamicEntityType derivedType;

	// Helper method for rotating.
	void yaw(double radians);

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
