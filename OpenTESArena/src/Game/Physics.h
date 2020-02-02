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

// Static class for physics-related calculations like ray casting.

class VoxelGrid;

class Physics
{
public:
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
			int id;
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
		void initEntity(double t, const Double3 &point, int id);

		double getT() const;
		double getTSqr() const;
		const Double3 &getPoint() const;
		Hit::Type getType() const;
		const VoxelHit &getVoxelHit() const;
		const EntityHit &getEntityHit() const;

		void setT(double t);
	};
private:
	Physics() = delete;
	~Physics() = delete;

	using VoxelEntityMap = std::unordered_map<
		Int3, std::vector<EntityManager::EntityVisibilityData>>;

	// Builds a set of voxels that are at least partially touched by entities. Ignores entities
	// behind the camera.
	static VoxelEntityMap makeVoxelEntityMap(const Double3 &cameraPosition,
		const Double3 &cameraDirection, double ceilingHeight, const VoxelGrid &voxelGrid,
		const EntityManager &entityManager);

	// Checks an initial voxel for ray hits and writes them into the output parameter.
	// Returns true if the ray hit something.
	static bool testInitialVoxelRay(const Double3 &rayStart, const Double3 &rayDirection,
		const Int3 &voxel, VoxelFacing farFacing, const Double3 &farPoint, double ceilingHeight,
		const VoxelGrid &voxelGrid, Physics::Hit &hit);

	// Checks a voxel for ray hits and writes them into the output parameter. Returns
	// true if the ray hit something.
	static bool testVoxelRay(const Double3 &rayStart, const Double3 &rayDirection,
		const Int3 &voxel, VoxelFacing nearFacing, const Double3 &nearPoint,
		const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid,
		Physics::Hit &hit);

	// Helper function for testing which entities in a voxel are intersected by a ray.
	static bool testEntitiesInVoxel(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &flatForward, const Double3 &flatRight, const Double3 &flatUp,
		const Int3 &voxel, const VoxelEntityMap &voxelEntityMap, bool pixelPerfect,
		const EntityManager &entityManager, const Renderer &renderer, Physics::Hit &hit);

	// Internal ray casting loop for stepping through individual voxels and checking
	// ray intersections with voxel data and entities.
	static void rayCastInternal(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &cameraForward, double ceilingHeight, const VoxelGrid &voxelGrid,
		const VoxelEntityMap &voxelEntityMap, bool pixelPerfect,
		const EntityManager &entityManager, const Renderer &renderer, Physics::Hit &hit);
public:
	// @todo: bit mask elements for each voxel data type.

	// Casts a ray through the world and writes any intersection data into the output
	// parameter. Returns true if the ray hit something.
	static bool rayCast(const Double3 &rayStart, const Double3 &rayDirection, double ceilingHeight,
		const Double3 &cameraForward, bool pixelPerfect, bool includeEntities,
		const EntityManager &entityManager, const VoxelGrid &voxelGrid, const Renderer &renderer,
		Physics::Hit &hit);
	static bool rayCast(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &cameraForward, bool pixelPerfect, bool includeEntities,
		const EntityManager &entityManager, const VoxelGrid &voxelGrid, const Renderer &renderer,
		Physics::Hit &hit);
};

#endif
