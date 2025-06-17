#ifndef COMBAT_LOGIC_H
#define COMBAT_LOGIC_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "../Entities/EntityInstance.h"
#include "../World/Coord.h"

#include "components/utilities/Span.h"

class EntityChunkManager;
class Random;
class Renderer;
class VoxelChunkManager;

// Stores voxels and entities that can be considered for attack calculation.
struct CombatHitSearchResult
{
	static constexpr int MAX_HIT_COUNT = 16;

	WorldInt3 voxels[MAX_HIT_COUNT]; // For breaking doors.
	int voxelCount;

	EntityInstanceID entities[MAX_HIT_COUNT]; // For enemy NPCs.
	int entityCount;

	CombatHitSearchResult();

	Span<const WorldInt3> getVoxels() const;
	Span<const EntityInstanceID> getEntities() const;
};

namespace CombatLogic
{
	void getHitSearchResult(const WorldDouble3 &searchPoint, double searchRadius, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, CombatHitSearchResult *outHitSearchResult);

	void spawnHitVfx(const EntityDefinition &hitEntityDef, const WorldDouble3 &position, EntityChunkManager &entityChunkManager,
		Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
}

#endif
