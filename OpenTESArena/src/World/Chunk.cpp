#include <algorithm>

#include "Chunk.h"

template <size_t T>
Chunk<T>::Chunk()
{
	std::fill(this->voxels.begin(), this->voxels.end(), 0);
}

template <size_t T>
constexpr int Chunk<T>::getWidth() const
{
	return WIDTH;
}

template <size_t T>
constexpr int Chunk<T>::getHeight() const
{
	return HEIGHT;
}

template <size_t T>
constexpr int Chunk<T>::getDepth() const
{
	return DEPTH;
}

template <size_t T>
constexpr int Chunk<T>::getIndex(int x, int y, int z) const
{
	return x + (y * this->getWidth()) + (z * this->getWidth() * this->getHeight());
}

template <size_t T>
uint16_t Chunk<T>::get(int x, int y, int z) const
{
	const int index = this->getIndex(x, y, z);
	return this->voxels.at(index);
}

template<size_t T>
void Chunk<T>::set(int x, int y, int z, uint16_t value)
{
	const int index = this->getIndex(x, y, z);
	this->voxels.at(index) = value;
}

// Template instantiations.
template class Chunk<3>;
template class Chunk<6>;
