#ifndef PHYSICS_H
#define PHYSICS_H

#include <unordered_map>
#include <vector>

#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Rendering/Renderer.h"
#include "../World/VoxelData.h"

// Static class for physics-related calculations like ray casting.

class VoxelGrid;

class Physics
{
public:
	// Intersection data for ray casts.
	struct Hit
	{
		enum class Type { Voxel, Entity };

		// @todo: voxel and entity structs so both structs can be in one union.
		// @todo: make the structs private and make init() methods so we never forget to assign
		// anything anywhere.

		double t;
		Double3 point;
		Int3 voxel;
		VoxelData::Facing facing;
		Hit::Type type;
		union { uint16_t voxelID; int entityID; };
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
	static bool testInitialVoxelRay(const Double3 &rayStart, const Double3 &direction,
		const Int3 &voxel, VoxelData::Facing facing, const Double3 &nearPoint,
		const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid,
		Physics::Hit &hit);

	// Checks a voxel for ray hits and writes them into the output parameter. Returns
	// true if the ray hit something.
	static bool testVoxelRay(const Double3 &rayStart, const Double3 &direction,
		const Int3 &voxel, VoxelData::Facing facing, const Double3 &nearPoint,
		const Double3 &farPoint, double ceilingHeight, const VoxelGrid &voxelGrid,
		Physics::Hit &hit);
public:
	// @todo: bit mask elements for each voxel data type.

	// Casts a ray through the world and writes any intersection data into the output
	// parameter. Returns true if the ray hit something.
	static bool rayCast(const Double3 &rayStart, const Double3 &rayDirection, double ceilingHeight,
		const Double3 &cameraForward, bool pixelPerfect, const EntityManager &entityManager,
		const VoxelGrid &voxelGrid, const Renderer &renderer, Physics::Hit &hit);
	static bool rayCast(const Double3 &rayStart, const Double3 &rayDirection,
		const Double3 &cameraForward, bool pixelPerfect, const EntityManager &entityManager,
		const VoxelGrid &voxelGrid, const Renderer &renderer, Physics::Hit &hit);
};

#endif
