#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

// Namespace for various platform-specific functions.
namespace Platform
{
	const std::string Windows = "Windows";
	const std::string Linux = "Linux";
	const std::string macOS = "Mac OS X";

	// Gets the current platform name via SDL_GetPlatform().
	std::string getPlatform();

	// Gets the base path to the executable (mostly intended for macOS .app).
	std::string getBasePath();

	// Gets the options folder path.
	std::string getOptionsPath();

	// Gets the screenshot folder path via SDL_GetPrefPath().
	std::string getScreenshotPath();

	// Gets the log folder path for logging program messages.
	std::string getLogPath();

	// Gets the default pixels-per-inch value from the OS.
	double getDefaultDPI();

	// Gets the max number of threads available on the CPU.
	int getThreadCount();

	// Gets the CPU cache line size in bytes. Important for things like avoiding false sharing
	// between threads that access the same cache line of memory.
	int getCacheLineSize();

	// Gets CPU support for 4-wide float vector intrinsics.
	bool hasSSE();

	// Gets CPU support for 8-wide float vector intrinsics.
	bool hasAVX();
}

#endif
