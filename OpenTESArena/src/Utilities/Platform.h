#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

// Static class for various platform-specific functions.

class Platform
{
private:
	Platform() = delete;
	~Platform() = delete;
public:
	// Gets the current platform name via SDL_GetPlatform().
	static std::string getPlatform();

	// Gets the base path to the executable (mostly intended for macOS .app).
	static std::string getBasePath();

	// Gets the options folder path via SDL_GetPrefPath().
	static std::string getOptionsPath();

	// Gets the screenshot folder path via SDL_GetPrefPath().
	static std::string getScreenshotPath();

	// Gets the log folder path for logging program messages.
	static std::string getLogPath();

	// Gets the max number of threads available on the CPU.
	static int getThreadCount();
};

#endif
