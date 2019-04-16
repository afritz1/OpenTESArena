#include <algorithm>

#include "ChunkSet.h"

template <typename T>
auto ChunkSet<T>::getIter(int x, int y, const std::vector<T> &chunks)
{
	const auto iter = std::find_if(this->chunks.begin(), this->chunks.end(),
		[x, y](const T &chunk)
	{
		return (chunk.getX() == x) && (chunk.getY() == y);
	});

	return iter;
}

template <typename T>
auto ChunkSet<T>::getIter(int x, int y, const std::vector<T> &chunks) const
{
	const auto iter = std::find_if(this->chunks.begin(), this->chunks.end(),
		[x, y](const T &chunk)
	{
		return (chunk.getX() == x) && (chunk.getY() == y);
	});

	return iter;
}

template <typename T>
int ChunkSet<T>::getCount() const
{
	return static_cast<int>(this->chunks.size());
}

template <typename T>
T *ChunkSet<T>::get(int x, int y)
{
	const auto iter = this->getIter(x, y, this->chunks);
	return (iter != this->chunks.end()) ? &(*iter) : nullptr;
}

template <typename T>
const T *ChunkSet<T>::get(int x, int y) const
{
	const auto iter = this->getIter(x, y, this->chunks);
	return (iter != this->chunks.end()) ? &(*iter) : nullptr;
}

template <typename T>
T *ChunkSet<T>::get(int index)
{
	return (index < this->getCount()) ? &this->chunks.at(index) : nullptr;
}

template <typename T>
const T *ChunkSet<T>::get(int index) const
{
	return (index < this->getCount()) ? &this->chunks.at(index) : nullptr;
}

template <typename T>
T &ChunkSet<T>::insert(int x, int y)
{
	const auto iter = this->getIter(x, y, this->chunks);

	// Add if it doesn't exist, overwrite if it does.
	const bool exists = iter != this->chunks.end();

	if (exists)
	{
		*iter = T(x, y);
		return *iter;
	}
	else
	{
		this->chunks.push_back(T(x, y));
		return this->chunks.back();
	}
}

template <typename T>
void ChunkSet<T>::remove(int x, int y)
{
	const auto iter = this->getIter(x, y, this->chunks);

	// Remove if the chunk exists.
	const bool exists = iter != this->chunks.end();

	if (exists)
	{
		this->chunks.erase(iter);
	}
}

// Template instantiations.
template class ChunkSet<InteriorChunk>;
template class ChunkSet<ExteriorChunk>;
