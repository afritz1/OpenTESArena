#include <algorithm>
#include <random>

#include "Random.h"
#include "RandomUtils.h"

#include "components/debug/Debug.h"

void RandomUtils::shuffle(BufferView<int> buffer)
{
	std::random_device randomDevice;
	std::shuffle(buffer.begin(), buffer.end(), std::default_random_engine(randomDevice()));
}

void RandomUtils::shuffle(BufferView<int> buffer, Random &random)
{
	const int count = buffer.getCount();
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
