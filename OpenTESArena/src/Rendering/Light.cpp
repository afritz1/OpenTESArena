#include <algorithm>

#include "Light.h"

Light::Light(const Double3 &point, const Double3 &color, double intensity)
	: point(point), color(color)
{
	this->intensity = intensity;
}

Light::~Light()
{

}

const Double3 &Light::getPoint() const
{
	return this->point;
}

const Double3 &Light::getColor() const
{
	return this->color;
}

double Light::getIntensity() const
{
	return this->intensity;
}

std::pair<Float3, Float3> Light::getAABB() const
{
	const float halfIntensity = static_cast<float>(this->intensity * 0.5);
	const float pointX = static_cast<float>(this->point.x);
	const float pointY = static_cast<float>(this->point.y);
	const float pointZ = static_cast<float>(this->point.z);

	const float minX = pointX - halfIntensity;
	const float minY = pointY - halfIntensity;
	const float minZ = pointZ - halfIntensity;

	const float maxX = pointX + halfIntensity;
	const float maxY = pointY + halfIntensity;
	const float maxZ = pointZ + halfIntensity;

	return std::make_pair(
		Float3(minX, minY, minZ),
		Float3(maxX, maxY, maxZ));
}

std::vector<Int3> Light::getTouchedVoxels(int worldWidth, int worldHeight, int worldDepth) const
{
	// Quick and easy solution: just enclose the light's reach within a box and get 
	// all voxels touching that box. A bit of wasted effort when rendering, though.
	const std::pair<Float3, Float3> aabb = this->getAABB();
	const Float3 &boxMin = aabb.first;
	const Float3 &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate clamped within 
	// world bounds.
	auto getClampedCoordinate = [worldWidth, worldHeight, worldDepth](const Float3 &point)
	{
		const int pX = static_cast<int>(point.x);
		const int pY = static_cast<int>(point.y);
		const int pZ = static_cast<int>(point.z);

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
	for (int k = voxelMin.z; k <= voxelMax.z; ++k)
	{
		for (int j = voxelMin.y; j <= voxelMax.y; ++j)
		{
			for (int i = voxelMin.x; i <= voxelMax.x; ++i)
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
	const std::pair<Float3, Float3> aabb = this->getAABB();
	const Float3 &boxMin = aabb.first;
	const Float3 &boxMax = aabb.second;

	// Lambda to convert a 3D point to a voxel coordinate.
	auto getVoxelCoordinate = [](const Float3 &point)
	{
		return Int3(
			static_cast<int>(point.x),
			static_cast<int>(point.y),
			static_cast<int>(point.z));
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getVoxelCoordinate(boxMin);
	const Int3 voxelMax = getVoxelCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.z; k <= voxelMax.z; ++k)
	{
		for (int j = voxelMin.y; j <= voxelMax.y; ++j)
		{
			for (int i = voxelMin.x; i <= voxelMax.x; ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;
}
