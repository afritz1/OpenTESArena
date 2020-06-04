#include <algorithm>
#include <random>

#include "Random.h"
#include "RandomUtils.h"

#include "components/debug/Debug.h"

void RandomUtils::shuffle(int *buffer, int count)
{
	DebugAssert(buffer != nullptr);

	int *bufferBegin = buffer;
	int *bufferEnd = bufferBegin + count;
	std::random_device randomDevice;
	std::shuffle(bufferBegin, bufferEnd, std::default_random_engine(randomDevice()));
}

void RandomUtils::shuffle(int *buffer, int count, Random &random)
{
	DebugAssert(buffer != nullptr);

	for (int i = 0; i < count - 1; i++)
	{
		const int swapIndex = i + random.next(count - i);
		if (swapIndex != i)
		{
			const int temp = buffer[swapIndex];
			buffer[swapIndex] = buffer[i];
			buffer[i] = temp;
		}
	}
}
