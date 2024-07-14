#ifndef PROFILER_H
#define PROFILER_H

#include <string>

namespace Profiler
{
	void startFrame();
	void setStart(const char *sampleName);
	void setStop(const char *sampleName, bool accumulate = false);
	std::string getResultsString();
}

#endif
