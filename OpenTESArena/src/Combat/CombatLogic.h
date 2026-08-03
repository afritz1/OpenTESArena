#pragma once

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "../Entities/EntityInstance.h"
#include "../World/Coord.h"

#include "components/utilities/Span.h"

class AudioManager;
class EntityChunkManager;
class Game;
class Random;
class Renderer;
class VoxelChunkManager;

enum class CombatResultSourceType;

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
	bool canSourceTypeDamageDoor(CombatResultSourceType sourceType);

	void getHitSearchResult(const WorldDouble3 &searchPoint, double searchRadius, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, CombatHitSearchResult *outHitSearchResult);

	static constexpr int SPELL_PROJECTILE_TYPE_COUNT = 12; // For animation lookups

	EntityDefID getBowProjectileEntityDefID();
	EntityDefID getSpellProjectileEntityDefID(int spellIndex);
	EntityDefID getSpellExplosionEntityDefID(int spellIndex);

	void spawnBowProjectile(WorldDouble3 position, Double3 direction, EntityChunkManager &entityChunkManager,
		Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
	void spawnSpellProjectile(int spellIndex, WorldDouble3 position, Double3 direction, EntityChunkManager &entityChunkManager, Random &random,
		AudioManager &audioManager, JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
	void spawnSpellExplosion(int spellIndex, WorldDouble3 position, EntityChunkManager &entityChunkManager, Random &random,
		AudioManager &audioManager, JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
	void spawnHitVfx(const EntityDefinition &hitEntityDef, const WorldDouble3 &position, EntityChunkManager &entityChunkManager,
		Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer);

	void onVoxelHitByPlayer(WorldInt3 hitWorldVoxel, CombatResultSourceType sourceType, Game &game);
	void onEntityHitByPlayer(EntityInstanceID hitEntityInstID, CombatResultSourceType sourceType, Game &game);
}
