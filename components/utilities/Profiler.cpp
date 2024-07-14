#include <algorithm>
#include <chrono>
#include <cstring>
#include <optional>
#include <string_view>

#include "Profiler.h"
#include "String.h"
#include "StringView.h"
#include "../debug/Debug.h"

namespace
{
	struct ProfilerSample
	{
		char name[128];
		std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
		double totalSeconds;

		ProfilerSample()
		{
			std::fill(std::begin(this->name), std::end(this->name), '\0');
			this->totalSeconds = 0.0;
		}

		void init(const char *name)
		{
			std::snprintf(this->name, std::size(this->name), "%s", name);
			this->startTime = std::chrono::time_point<std::chrono::high_resolution_clock>();
			this->totalSeconds = 0.0;
		}
	};

	ProfilerSample g_samples[64];
	int g_sampleCount = 0;

	ProfilerSample *GetSample(const char *sampleName)
	{
		if (String::isNullOrEmpty(sampleName))
		{
			return nullptr;
		}

		for (int i = 0; i < g_sampleCount; i++)
		{
			ProfilerSample &sample = g_samples[i];
			if (StringView::equals(sampleName, sample.name))
			{
				return &sample;
			}
		}

		return nullptr;
	}

	ProfilerSample *GetOrAddSample(const char *sampleName)
	{
		ProfilerSample *sample = GetSample(sampleName);
		if (sample != nullptr)
		{
			return sample;
		}

		constexpr int maxSampleCount = static_cast<int>(std::size(g_samples));
		if (g_sampleCount == maxSampleCount)
		{
			return nullptr;
		}

		ProfilerSample &newSample = g_samples[g_sampleCount];
		newSample.init(sampleName);
		g_sampleCount++;
		return &newSample;
	}

	double SecondsToMilliseconds(double seconds)
	{
		return seconds * 1000.0;
	}

	std::string DurationToString(double time)
	{
		char buffer[32];
		std::snprintf(buffer, std::size(buffer), "%.2f", time);
		return std::string(buffer);
	}
}

void Profiler::startFrame()
{
	g_sampleCount = 0;
}

void Profiler::setStart(const char *sampleName)
{
	ProfilerSample *sample = GetOrAddSample(sampleName);
	if (sample == nullptr)
	{
		DebugLogError("Couldn't start sample \"" + std::string(sampleName) + "\".");
		return;
	}

	sample->startTime = std::chrono::high_resolution_clock::now();
}

void Profiler::setStop(const char *sampleName, bool accumulate)
{
	const auto endTime = std::chrono::high_resolution_clock::now();
	ProfilerSample *sample = GetSample(sampleName);
	if (sample == nullptr)
	{
		DebugLogError("Couldn't stop sample \"" + std::string(sampleName) + "\".");
		return;
	}

	const std::chrono::nanoseconds diff = endTime - sample->startTime;
	const double seconds = static_cast<double>(diff.count()) / static_cast<double>(std::nano::den);
	if (accumulate)
	{
		sample->totalSeconds += seconds;
	}
	else
	{
		sample->totalSeconds = seconds;
	}
}

std::string Profiler::getResultsString()
{
	// @todo: implement a nicer hierarchy printing, probably need at least a struct and some kind of namespace trimming by '.'

	std::string resultsStr;
	for (int i = 0; i < g_sampleCount; i++)
	{
		const ProfilerSample &sample = g_samples[i];

		if (!resultsStr.empty())
		{
			resultsStr += '\n';
		}

		resultsStr += sample.name;
		resultsStr += ": ";
		resultsStr += DurationToString(SecondsToMilliseconds(sample.totalSeconds));
		resultsStr += "ms";
	}

	return resultsStr;
}
