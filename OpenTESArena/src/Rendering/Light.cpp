#include <algorithm>

#include "Light.h"

#include "../Math/Int3.h"

Light::Light(const Float3d &point, const Float3d &color, double intensity)
	: point(point), color(color)
{
	this->intensity = intensity;
}

Light::~Light()
{

}

const Float3d &Light::getPoint() const
{
	return this->point;
}

const Float3d &Light::getColor() const
{
	return this->color;
}

double Light::getIntensity() const
{
	return this->intensity;
}

std::pair<Float3f, Float3f> Light::getAABB() const
{
	const float halfIntensity = static_cast<float>(this->intensity * 0.5);

	const float minX = this->point.getX() - halfIntensity;
	const float minY = this->point.getY() - halfIntensity;
	const float minZ = this->point.getZ() - halfIntensity;

	const float maxX = this->point.getX() + halfIntensity;
	const float maxY = this->point.getY() + halfIntensity;
	const float maxZ = this->point.getZ() + halfIntensity;

	return std::make_pair(
		Float3f(minX, minY, minZ),
		Float3f(maxX, maxY, maxZ));
}

std::vector<Int3> Light::getTouchedVoxels(int worldWidth, int worldHeight, int worldDepth) const
{
	// Quick and easy solution: just enclose the light's reach within a box and get 
	// all voxels touching that box. A bit of wasted effort when rendering, though.
	const std::pair<Float3f, Float3f> aabb = this->getAABB();
	const Float3f &boxMin = aabb.first;
	const Float3f &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate clamped within 
	// world bounds.
	auto getClampedCoordinate = [worldWidth, worldHeight, worldDepth](const Float3f &point)
	{
		const int pX = static_cast<int>(point.getX());
		const int pY = static_cast<int>(point.getY());
		const int pZ = static_cast<int>(point.getZ());

		return Int3(
			std::max(0, std::min(worldWidth - 1, pX)),
			std::max(0, std::min(worldHeight - 1, pY)),
			std::max(0, std::min(worldDepth - 1, pZ)));
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getClampedCoordinate(boxMin);
	const Int3 voxelMax = getClampedCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.getZ(); k <= voxelMax.getZ(); ++k)
	{
		for (int j = voxelMin.getY(); j <= voxelMax.getY(); ++j)
		{
			for (int i = voxelMin.getX(); i <= voxelMax.getX(); ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;

	// A smarter version would treat the light's reach like a sphere and get all
	// voxels touched by that sphere instead.
}

std::vector<Int3> Light::getTouchedVoxels() const
{
	const std::pair<Float3f, Float3f> aabb = this->getAABB();
	const Float3f &boxMin = aabb.first;
	const Float3f &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate.
	auto getVoxelCoordinate = [](const Float3f &point)
	{
		return Int3(
			static_cast<int>(point.getX()),
			static_cast<int>(point.getY()),
			static_cast<int>(point.getZ()));
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getVoxelCoordinate(boxMin);
	const Int3 voxelMax = getVoxelCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.getZ(); k <= voxelMax.getZ(); ++k)
	{
		for (int j = voxelMin.getY(); j <= voxelMax.getY(); ++j)
		{
			for (int i = voxelMin.getX(); i <= voxelMax.getX(); ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;
}
