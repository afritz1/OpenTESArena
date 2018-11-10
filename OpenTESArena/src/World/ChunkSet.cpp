#include <algorithm>

#include "ChunkSet.h"

template <typename T>
auto ChunkSet<T>::getIter(const Int2 &point, const ChunkList &chunks)
{
	const auto iter = std::find_if(this->chunks.begin(), this->chunks.end(),
		[&point](const auto &pair)
	{
		return pair.first == point;
	});

	return iter;
}

template <typename T>
auto ChunkSet<T>::getIter(const Int2 &point, const ChunkList &chunks) const
{
	const auto iter = std::find_if(this->chunks.begin(), this->chunks.end(),
		[&point](const auto &pair)
	{
		return pair.first == point;
	});

	return iter;
}

template<typename T>
int ChunkSet<T>::getCount() const
{
	return static_cast<int>(this->chunks.size());
}

template <typename T>
T *ChunkSet<T>::get(const Int2 &point)
{
	const auto iter = this->getIter(point, this->chunks);
	return (iter != this->chunks.end()) ? &iter->second : nullptr;
}

template <typename T>
const T *ChunkSet<T>::get(const Int2 &point) const
{
	const auto iter = this->getIter(point, this->chunks);
	return (iter != this->chunks.end()) ? &iter->second : nullptr;
}

template <typename T>
typename ChunkSet<T>::PairType *ChunkSet<T>::getIndex(int index)
{
	return (index < this->getCount()) ? &this->chunks.at(index) : nullptr;
}

template <typename T>
typename const ChunkSet<T>::PairType *ChunkSet<T>::getIndex(int index) const
{
	return (index < this->getCount()) ? &this->chunks.at(index) : nullptr;
}

template <typename T>
void ChunkSet<T>::set(const Int2 &point, const T &chunk)
{
	const auto iter = this->getIter(point, this->chunks);

	// Add if it doesn't exist, overwrite if it does.
	const bool exists = iter != this->chunks.end();

	if (exists)
	{
		*iter = std::make_pair(point, chunk);
	}
	else
	{
		this->chunks.push_back(std::make_pair(point, chunk));
	}
}

template <typename T>
void ChunkSet<T>::set(const Int2 &point, T &&chunk)
{
	const auto iter = this->getIter(point, this->chunks);

	// Add if it doesn't exist, overwrite if it does.
	const bool exists = iter != this->chunks.end();

	if (exists)
	{
		*iter = std::make_pair(point, std::move(chunk));
	}
	else
	{
		this->chunks.push_back(std::make_pair(point, std::move(chunk)));
	}
}

template<typename T>
void ChunkSet<T>::remove(const Int2 &point)
{
	const auto iter = this->getIter(point, this->chunks);

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
