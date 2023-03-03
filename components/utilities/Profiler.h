#ifndef PROFILER_H
#define PROFILER_H

#include <chrono>
#include <optional>
#include <string>

namespace ProfilerUtils
{
	const std::string ASSETS = "Assets";
	const std::string AUDIO = "Audio";
	const std::string COLLISION = "Collision";
	const std::string ENTITIES = "Entities";
	const std::string INPUT = "Input";
	const std::string RENDERING = "Rendering";
	const std::string SKY = "Sky";
	const std::string UI = "UI";
	const std::string VOXELS = "Voxels";
	const std::string WORLD = "World";
}

struct ProfilerSampler
{
	std::string name;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;

	void init(std::string &&name);
};

// For conveniently timing chunks of code.
class Profiler
{
private:
	static constexpr int MAX_SAMPLERS = 16;

	ProfilerSampler samplers[MAX_SAMPLERS];
	int samplerCount;

	std::optional<int> tryGetSamplerIndex(const std::string &name) const;
	std::string durationToString(double time) const;
public:
	Profiler();

	int getSamplerCount() const;
	const std::string &getSamplerName(int index) const;

	double getSeconds(const std::string &name) const;
	std::string getSecondsString(const std::string &name) const;
	double getMilliseconds(const std::string &name) const;
	std::string getMillisecondsString(const std::string &name) const;

	void setStart(const std::string &name);
	void setStop(const std::string &name);
	void clear();
};

#endif
