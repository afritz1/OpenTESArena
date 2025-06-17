#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <algorithm>
#include <random>

#include "Random.h"

#include "components/utilities/Span.h"

namespace RandomUtils
{
	// Shuffles the elements randomly with an unspecified RNG source.
	template<typename T>
	void shuffle(Span<T> buffer)
	{
		std::random_device randomDevice;
		std::shuffle(buffer.begin(), buffer.end(), std::default_random_engine(randomDevice()));
	}

	// Shuffles the elements randomly with the given RNG source.
	template<typename T>
	void shuffle(Span<T> buffer, Random &random)
	{
		const int count = buffer.getCount();
		for (int i = 0; i < count - 1; i++)
		{
			const int swapIndex = i + random.next(count - i);
			if (swapIndex != i)
			{
				const T temp = buffer[swapIndex];
				buffer[swapIndex] = buffer[i];
				buffer[i] = temp;
			}
		}
	}
}

#endif
