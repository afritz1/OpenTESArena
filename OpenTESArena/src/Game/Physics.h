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
#include "../World/VoxelDefinition.h"
#include "../World/VoxelUtils.h"

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
			CoordInt3 coord;
			std::optional<VoxelFacing3D> facing;
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
		static constexpr double MAX_T = std::numeric_limits<double>::infinity();

		void initVoxel(double t, const Double3 &point, uint16_t id, const CoordInt3 &coord,
			const VoxelFacing3D *facing);
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
	bool rayCast(const CoordDouble3 &rayStart, const NewDouble3 &rayDirection, int chunkDistance,
		double ceilingHeight, const NewDouble3 &cameraForward, bool pixelPerfect, const Palette &palette,
		bool includeEntities, const LevelData &levelData, const EntityDefinitionLibrary &entityDefLibrary,
		const Renderer &renderer, Physics::Hit &hit);
	bool rayCast(const CoordDouble3 &rayStart, const NewDouble3 &rayDirection, int chunkDistance,
		const NewDouble3 &cameraForward, bool pixelPerfect, const Palette &palette, bool includeEntities,
		const LevelData &levelData, const EntityDefinitionLibrary &entityDefLibrary, const Renderer &renderer,
		Physics::Hit &hit);
};

#endif
