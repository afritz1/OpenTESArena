#include <algorithm>
#include <cmath>
#include <numeric>
#include <tuple>

#include "FPSCounter.h"
#include "../debug/Debug.h"

FPSCounter::FPSCounter()
{
	this->frameTimes.fill(0.0);
}

int FPSCounter::getFrameCount() const
{
	return static_cast<int>(this->frameTimes.size());
}

double FPSCounter::getFrameTime(int index) const
{
	DebugAssertIndex(this->frameTimes, index);
	return this->frameTimes[index];
}

double FPSCounter::getAverageFrameTime() const
{
	// Only need a few frame times to get a decent approximation.
	constexpr size_t count = 20;
	static_assert(std::tuple_size<decltype(this->frameTimes)>::value >= count);

	const double sum = std::accumulate(this->frameTimes.begin(), this->frameTimes.begin() + count, 0.0);
	return sum / static_cast<double>(count);
}

double FPSCounter::getAverageFPS() const
{
	return 1.0 / this->getAverageFrameTime();
}

void FPSCounter::updateFrameTime(double dt)
{
	// Rotate the array right by one index (this puts the last value at the front).
	std::rotate(this->frameTimes.rbegin(),
		this->frameTimes.rbegin() + 1, this->frameTimes.rend());

	this->frameTimes.front() = dt;
}
