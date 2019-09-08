#ifndef PROFILER_H
#define PROFILER_H

#include <chrono>
#include <string>
#include <vector>

class Profiler
{
public:
	class Sampler
	{
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;
	public:
		double getSeconds() const;
		double getMilliseconds() const;

		void setStart();
		void setStop();
	};
private:
	std::vector<std::pair<std::string, Sampler>> samplers;

	// Searches samplers for the one with the given name.
	Sampler *findSampler(const std::string &name);
public:
	// Overwrites existing (if any).
	Sampler *addSampler(const std::string &name);

	// Gets sampler from the given name, or null if it doesn't exist.
	Sampler *getSampler(const std::string &name);

	// Removes sampler if it exists.
	void removeSampler(const std::string &name);

	// Clears all samplers from the profiler.
	void clear();
};

#endif
