#include <algorithm>

#include "Profiler.h"
#include "../debug/Debug.h"

double Profiler::Sampler::getSeconds() const
{
	return static_cast<double>((this->endTime - this->startTime).count()) /
		static_cast<double>(std::nano::den);
}

double Profiler::Sampler::getMilliseconds() const
{
	return this->getSeconds() * 1000.0;
}

void Profiler::Sampler::setStart()
{
	this->startTime = std::chrono::high_resolution_clock::now();
}

void Profiler::Sampler::setStop()
{
	this->endTime = std::chrono::high_resolution_clock::now();
}

Profiler::Sampler *Profiler::findSampler(const std::string &name)
{
	const auto iter = std::find_if(this->samplers.begin(), this->samplers.end(),
		[&name](const auto &pair)
	{
		return pair.first == name;
	});

	return (iter != this->samplers.end()) ? iter->second.get() : nullptr;
}

Profiler::Sampler *Profiler::addSampler(const std::string &name)
{
	DebugAssertMsg(this->findSampler(name) == nullptr, "Sampler \"" + name + "\" already exists.");
	this->samplers.push_back(std::make_pair(name, std::make_unique<Sampler>()));
	return this->samplers.back().second.get();
}

Profiler::Sampler *Profiler::getSampler(const std::string &name)
{
	return this->findSampler(name);
}

void Profiler::removeSampler(const std::string &name)
{
	const auto iter = std::find_if(this->samplers.begin(), this->samplers.end(),
		[&name](const auto &pair)
	{
		return pair.first == name;
	});

	if (iter != this->samplers.end())
	{
		this->samplers.erase(iter);
	}
}

void Profiler::clear()
{
	this->samplers.clear();
}
