#ifndef PHYSICS_H
#define PHYSICS_H

#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Rendering/Renderer.h"
#include "../Voxels/VoxelUtils.h"

// Namespace for physics-related calculations like ray casting.

class LevelInstance;

namespace Physics
{
	// Intersection data for ray casts.
	class Hit
	{
	public:
		enum class Type { Voxel, Entity };

		struct VoxelHit
		{
			VoxelInt3 voxel;
			std::optional<VoxelFacing3D> facing;
		};

		struct EntityHit
		{
			EntityID id;
			EntityType type;
		};
	private:
		double t;
		CoordDouble3 coord; // Hit point in 3D space.
		Hit::Type type;

		// Not in a union so VoxelHit can use std::optional.
		VoxelHit voxelHit;
		EntityHit entityHit;
	public:
		static constexpr double MAX_T = std::numeric_limits<double>::infinity();

		void initVoxel(double t, const CoordDouble3 &coord, const VoxelInt3 &voxel, const VoxelFacing3D *facing);
		void initEntity(double t, const CoordDouble3 &coord, EntityID id, EntityType type);

		double getT() const;
		double getTSqr() const;
		const CoordDouble3 &getCoord() const;
		Hit::Type getType() const;
		const VoxelHit &getVoxelHit() const;
		const EntityHit &getEntityHit() const;

		void setT(double t);
	};

	// @todo: bit mask elements for each voxel data type.

	// Casts a ray through the world and writes any intersection data into the output parameter. Returns true
	// if the ray hit something.
	bool rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, double ceilingScale,
		const VoxelDouble3 &cameraForward, bool includeEntities, const LevelInstance &levelInst,
		const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer, Physics::Hit &hit);
	bool rayCast(const CoordDouble3 &rayStart, const VoxelDouble3 &rayDirection, const VoxelDouble3 &cameraForward,
		bool includeEntities, const LevelInstance &levelInst, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, Physics::Hit &hit);
};

#endif
