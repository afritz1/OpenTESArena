#ifndef PHYSICS_H
#define PHYSICS_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Rendering/Renderer.h"
#include "../World/VoxelDefinition.h"

// Namespace for physics-related calculations like ray casting.

class VoxelGrid;

namespace Physics
{
	// Intersection data for ray casts.
	class Hit
	{
	public:
		enum class Type { Voxel, Entity };

		struct VoxelHit
		{
			uint16_t id;
			Int3 voxel;
			std::optional<VoxelFacing> facing;
		};

		struct EntityHit
		{
			EntityID id;
			EntityType type;
		};
	private:
		double t;
		Double3 point;
		Hit::Type type;

		// Not in a union so VoxelHit can use std::optional.
		VoxelHit voxelHit;
		EntityHit entityHit;
	public:
		static const double MAX_T;

		void initVoxel(double t, const Double3 &point, uint16_t id, const Int3 &voxel,
			const VoxelFacing *facing);
		void initEntity(double t, const Double3 &point, EntityID id, EntityType type);

		double getT() const;
		double getTSqr() const;
		const Double3 &getPoint() const;
		Hit::Type getType() const;
		const VoxelHit &getVoxelHit() const;
		const EntityHit &getEntityHit() const;

		void setT(double t);
	};

	// @todo: bit mask elements for each voxel data type.

	// Casts a ray through the world and writes any intersection data into the output
	// parameter. Returns true if the ray hit something.
	bool rayCast(const Double3 &rayStart, const Double3 &rayDirection, int chunkDistance,
		double ceilingHeight, const LevelData::ChasmStates &chasmStates,
		const Double3 &cameraForward, bool pixelPerfect, bool includeEntities,
		const EntityManager &entityManager, const VoxelGrid &voxelGrid,
		const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer, Physics::Hit &hit);
	bool rayCast(const Double3 &rayStart, const Double3 &rayDirection, int chunkDistance,
		const LevelData::ChasmStates &chasmStates, const Double3 &cameraForward,
		bool pixelPerfect, bool includeEntities, const EntityManager &entityManager,
		const VoxelGrid &voxelGrid, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, Physics::Hit &hit);
};

#endif
