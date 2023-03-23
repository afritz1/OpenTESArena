#include <cstring>

#include "Profiler.h"
#include "../debug/Debug.h"

void ProfilerSampler::init(std::string &&name)
{
	this->name = std::move(name);
	this->startTime = std::chrono::high_resolution_clock::now();
	this->endTime = this->startTime;
}

Profiler::Profiler()
{
	this->samplerCount = 0;
}

std::optional<int> Profiler::tryGetSamplerIndex(const std::string &name) const
{
	for (int i = 0; i < this->samplerCount; i++)
	{
		if (this->samplers[i].name == name)
		{
			return i;
		}
	}

	return std::nullopt;
}

std::string Profiler::durationToString(double time) const
{
	char buffer[32];
	std::snprintf(buffer, std::size(buffer), "%.2f", time);
	return std::string(buffer);
}

int Profiler::getSamplerCount() const
{
	return this->samplerCount;
}

const std::string &Profiler::getSamplerName(int index) const
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->samplerCount);
	return this->samplers[index].name;
}

double Profiler::getSeconds(const std::string &name) const
{
	const std::optional<int> index = this->tryGetSamplerIndex(name);
	if (!index.has_value())
	{
		return 0.0;
	}

	const ProfilerSampler &sampler = this->samplers[*index];
	DebugAssertMsg(sampler.endTime >= sampler.startTime, "Invalid start/end times for \"" + name + "\".");
	const std::chrono::nanoseconds diff = sampler.endTime - sampler.startTime;
	return static_cast<double>(diff.count()) / static_cast<double>(std::nano::den);
}

std::string Profiler::getSecondsString(const std::string &name) const
{
	const double seconds = this->getSeconds(name);
	return this->durationToString(seconds);
}

double Profiler::getMilliseconds(const std::string &name) const
{
	return this->getSeconds(name) * 1000.0;
}

std::string Profiler::getMillisecondsString(const std::string &name) const
{
	const double milliseconds = this->getMilliseconds(name);
	return this->durationToString(milliseconds);
}

void Profiler::setStart(const std::string &name)
{
	ProfilerSampler *sampler = nullptr;
	std::optional<int> index = this->tryGetSamplerIndex(name);
	if (index.has_value())
	{
		sampler = &this->samplers[*index];
	}
	else
	{
		if (this->samplerCount == MAX_SAMPLERS)
		{
			DebugLogError("Too many samplers, can't add \"" + name + "\".");
			return;
		}

		index = this->samplerCount;
		sampler = &this->samplers[*index];
		sampler->init(std::string(name));

		this->samplerCount++;
	}

	sampler->startTime = std::chrono::high_resolution_clock::now();
}

void Profiler::setStop(const std::string &name)
{
	ProfilerSampler *sampler = nullptr;
	std::optional<int> index = this->tryGetSamplerIndex(name);
	if (index.has_value())
	{
		sampler = &this->samplers[*index];
	}
	else
	{
		DebugLogError("Couldn't find sampler for \"" + name + "\", need to start it first.");
		return;
	}

	sampler->endTime = std::chrono::high_resolution_clock::now();
}

void Profiler::clear()
{
	this->samplerCount = 0;
}
