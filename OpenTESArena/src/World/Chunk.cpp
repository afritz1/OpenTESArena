#include <cassert>

#include "Chunk.h"

template <size_t T>
Chunk<T>::Chunk(int x, int y)
{
	this->voxels.fill(0);
	this->voxelData.fill(VoxelData());

	// Let the first voxel data (air) be usable immediately. All default voxel IDs can safely point to it.
	this->voxelDataCount = 1;

	this->x = x;
	this->y = y;
}

template <size_t T>
int Chunk<T>::getX() const
{
	return this->x;
}

template <size_t T>
int Chunk<T>::getY() const
{
	return this->y;
}

template <size_t T>
constexpr int Chunk<T>::getWidth() const
{
	return Chunk<T>::WIDTH;
}

template <size_t T>
constexpr int Chunk<T>::getHeight() const
{
	return Chunk<T>::HEIGHT;
}

template <size_t T>
constexpr int Chunk<T>::getDepth() const
{
	return Chunk<T>::DEPTH;
}

template <size_t T>
constexpr bool Chunk<T>::coordIsValid(int x, int y, int z) const
{
	return (x >= 0) && (x < this->getWidth()) &&
		(y >= 0) && (y < this->getHeight()) &&
		(z >= 0) && (z < this->getDepth());
}

template <size_t T>
constexpr int Chunk<T>::getIndex(int x, int y, int z) const
{
	assert(this->coordIsValid(x, y, z));
	return x + (y * this->getWidth()) + (z * this->getWidth() * this->getHeight());
}

template <size_t T>
constexpr VoxelID Chunk<T>::get(int x, int y, int z) const
{
	const int index = this->getIndex(x, y, z);
	return this->voxels[index];
}

template <size_t T>
const VoxelData &Chunk<T>::getVoxelData(VoxelID id) const
{
	assert(id < this->voxelData.size());
	return this->voxelData[id];
}

template <size_t T>
constexpr void Chunk<T>::set(int x, int y, int z, VoxelID value)
{
	const int index = this->getIndex(x, y, z);
	this->voxels[index] = value;
}

template <size_t T>
VoxelID Chunk<T>::addVoxelData(VoxelData &&voxelData)
{
	// If we ever reach more than 256 unique voxel datas, we need more bits per voxel.
	assert(this->voxelDataCount < this->voxelData.size());

	this->voxelData[this->voxelDataCount] = std::move(voxelData);
	this->voxelDataCount++;
	return this->voxelDataCount - 1;
}

// Template instantiations.
template class Chunk<3>;
template class Chunk<6>;
