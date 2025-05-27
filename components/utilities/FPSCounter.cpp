#include <algorithm>
#include <cmath>
#include <numeric>
#include <tuple>

#include "FPSCounter.h"
#include "../debug/Debug.h"

FPSCounter::FPSCounter()
{
	std::fill(std::begin(this->frameTimes), std::end(this->frameTimes), 0.0);
}

int FPSCounter::getFrameCount() const
{
	return static_cast<int>(std::size(this->frameTimes));
}

double FPSCounter::getFrameTime(int index) const
{
	DebugAssertIndex(this->frameTimes, index);
	return this->frameTimes[index];
}

double FPSCounter::getAverageFrameTime() const
{
	// Only need a few frame times to get a decent approximation.
	const size_t count = std::size(this->frameTimes) / 2;

	const auto beginIter = std::begin(this->frameTimes);
	const auto endIter = beginIter + count;
	const double sum = std::accumulate(beginIter, endIter, 0.0);
	return sum / static_cast<double>(count);
}

double FPSCounter::getAverageFPS() const
{
	return 1.0 / this->getAverageFrameTime();
}

double FPSCounter::getHighestFPS() const
{
	const auto beginIter = std::begin(this->frameTimes);
	const auto endIter = std::end(this->frameTimes);
	const auto iter = std::min_element(beginIter, endIter);
	if (iter == endIter)
	{
		return 0.0;
	}

	return 1.0 / *iter;
}

double FPSCounter::getLowestFPS() const
{
	const auto beginIter = std::begin(this->frameTimes);
	const auto endIter = std::end(this->frameTimes);
	const auto iter = std::max_element(beginIter, endIter);
	if (iter == endIter)
	{
		return 0.0;
	}

	return 1.0 / *iter;
}

void FPSCounter::updateFrameTime(double dt)
{
	// Rotate right by one index so the last value becomes first, then overwrite it.
	const auto rbeginIter = std::rbegin(this->frameTimes);
	const auto rendIter = std::rend(this->frameTimes);
	std::rotate(rbeginIter, rbeginIter + 1, rendIter);

	this->frameTimes[0] = dt;
}
