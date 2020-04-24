#include <algorithm>
#include <random>

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
