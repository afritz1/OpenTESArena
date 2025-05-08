#ifndef COMBAT_LOGIC_H
#define COMBAT_LOGIC_H

#include "../Entities/EntityInstance.h"
#include "../World/Coord.h"

#include "components/utilities/BufferView.h"

class EntityChunkManager;
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

	BufferView<const WorldInt3> getVoxels() const;
	BufferView<const EntityInstanceID> getEntities() const;
};

namespace CombatLogic
{
	void getHitSearchResult(const WorldDouble3 &searchPoint, double searchRadius, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, CombatHitSearchResult *outHitSearchResult);
}

#endif
