#include <algorithm>
#include <cmath>
#include <numeric>

#include "FPSCounter.h"

FPSCounter::FPSCounter()
{
	this->frameTimes.fill(0.0);
}

FPSCounter::~FPSCounter()
{

}

double FPSCounter::getAverageFrameTime() const
{
	const double sum = std::accumulate(this->frameTimes.begin(), this->frameTimes.end(), 0.0);
	return sum / static_cast<double>(this->frameTimes.size());
}

double FPSCounter::getFPS() const
{
	const double fps = 1.0 / this->getAverageFrameTime();
	return std::isfinite(fps) ? fps : 0.0;
}

void FPSCounter::updateFrameTime(double dt)
{
	// Rotate the array right by one index (this puts the last value at the front).
	std::rotate(this->frameTimes.rbegin(),
		this->frameTimes.rbegin() + 1, this->frameTimes.rend());

	this->frameTimes.front() = dt;
}
