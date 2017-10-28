#include <cassert>

#include "VoxelBuilder.h"
#include "../Math/Vector3.h"
#include "../Rendering/Rect3D.h"

std::vector<Rect3D> VoxelBuilder::makeSizedBlock(int x, int y, int z, float y1, float y2)
{
	// Block height (thickness) must be positive.
	assert(y2 > y1);

	const float xPos = static_cast<float>(x);
	const float yPos = static_cast<float>(y);
	const float zPos = static_cast<float>(z);

	// Allocating a vector for each voxel is probably slow, so it should take
	// a vector reference with a size of 6 as a parameter instead.
	std::vector<Rect3D> rects;
	rects.reserve(6);

	// Front.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + y2, zPos),
		Float3(xPos + 1.0f, yPos + y1, zPos),
		Float3(xPos, yPos + y1, zPos)));

	// Back.
	rects.push_back(Rect3D(
		Float3(xPos, yPos + y2, zPos + 1.0f),
		Float3(xPos, yPos + y1, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos + y1, zPos + 1.0f)));

	// Top.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + y2, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos + y2, zPos),
		Float3(xPos, yPos + y2, zPos)));

	// Bottom.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos, zPos),
		Float3(xPos + 1.0f, yPos, zPos + 1.0f),
		Float3(xPos, yPos, zPos + 1.0f)));

	// Right.
	rects.push_back(Rect3D(
		Float3(xPos, yPos + y2, zPos),
		Float3(xPos, yPos + y1, zPos),
		Float3(xPos, yPos + y1, zPos + 1.0f)));

	// Left.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + y2, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos + y1, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos + y1, zPos)));

	return rects;
}

std::vector<Rect3D> VoxelBuilder::makeBlock(int x, int y, int z)
{
	return VoxelBuilder::makeSizedBlock(x, y, z, 0.0f, 1.0f);
}

Rect3D VoxelBuilder::makeCeiling(int x, int y, int z)
{
	const float xPos = static_cast<float>(x);
	const float yPos = static_cast<float>(y);
	const float zPos = static_cast<float>(z);

	return Rect3D(
		Float3(xPos + 1.0f, yPos + 1.0f, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos + 1.0f, zPos),
		Float3(xPos, yPos + 1.0f, zPos));
}

Rect3D VoxelBuilder::makeFloor(int x, int y, int z)
{
	const float xPos = static_cast<float>(x);
	const float yPos = static_cast<float>(y);
	const float zPos = static_cast<float>(z);

	return Rect3D(
		Float3(xPos + 1.0f, yPos, zPos),
		Float3(xPos + 1.0f, yPos, zPos + 1.0f),
		Float3(xPos, yPos, zPos + 1.0f));
}

std::vector<Rect3D> VoxelBuilder::makeHollowY(int x, int y, int z)
{
	const float xPos = static_cast<float>(x);
	const float yPos = static_cast<float>(y);
	const float zPos = static_cast<float>(z);

	// Allocating a vector for each voxel is probably slow, so it should take
	// a vector reference with a size of 4 as a parameter instead.
	std::vector<Rect3D> rects;
	rects.reserve(4);

	// Front.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + 1.0f, zPos),
		Float3(xPos + 1.0f, yPos, zPos),
		Float3(xPos, yPos, zPos)));

	// Back.
	rects.push_back(Rect3D(
		Float3(xPos, yPos + 1.0f, zPos + 1.0f),
		Float3(xPos, yPos, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos, zPos + 1.0f)));

	// Right.
	rects.push_back(Rect3D(
		Float3(xPos, yPos + 1.0f, zPos),
		Float3(xPos, yPos, zPos),
		Float3(xPos, yPos, zPos + 1.0f)));

	// Left.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + 1.0f, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos, zPos)));

	return rects;
}

std::vector<Rect3D> VoxelBuilder::makeHollowYZ(int x, int y, int z)
{
	const float xPos = static_cast<float>(x);
	const float yPos = static_cast<float>(y);
	const float zPos = static_cast<float>(z);

	// Allocating a vector for each voxel is probably slow, so it should take
	// a vector reference with a size of 2 as a parameter instead.
	std::vector<Rect3D> rects;
	rects.reserve(2);

	// Right.
	rects.push_back(Rect3D(
		Float3(xPos, yPos + 1.0f, zPos),
		Float3(xPos, yPos, zPos),
		Float3(xPos, yPos, zPos + 1.0f)));

	// Left.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + 1.0f, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos, zPos)));

	return rects;
}

std::vector<Rect3D> VoxelBuilder::makeHollowXY(int x, int y, int z)
{
	const float xPos = static_cast<float>(x);
	const float yPos = static_cast<float>(y);
	const float zPos = static_cast<float>(z);

	// Allocating a vector for each voxel is probably slow, so it should take
	// a vector reference with a size of 2 as a parameter instead.
	std::vector<Rect3D> rects;
	rects.reserve(2);

	// Front.
	rects.push_back(Rect3D(
		Float3(xPos + 1.0f, yPos + 1.0f, zPos),
		Float3(xPos + 1.0f, yPos, zPos),
		Float3(xPos, yPos, zPos)));

	// Back.
	rects.push_back(Rect3D(
		Float3(xPos, yPos + 1.0f, zPos + 1.0f),
		Float3(xPos, yPos, zPos + 1.0f),
		Float3(xPos + 1.0f, yPos, zPos + 1.0f)));

	return rects;
}
