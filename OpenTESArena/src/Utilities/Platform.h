#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

// Static class for various platform-specific functions.

class Platform
{
private:
	// Linux user environment variables. Data home is "~/.local/share" and config home is
	// "~/.config". If getenv("XDG_...") is null, then try getenv("HOME") with the desired
	// subdirectory appended (i.e., "./local/share").
	static const std::string XDGDataHome;
	static const std::string XDGConfigHome;

	Platform() = delete;
	~Platform() = delete;

	// Gets the user's home environment variable ($HOME). Does not have a trailing slash.
	static std::string getHomeEnv();

	// Gets the data home directory from $XDG_DATA_HOME (or $HOME/.local/share as a fallback).
	// Does not have a trailing slash. Basically the same as SDL_GetPrefPath() on Linux systems
	// (without the slash).
	static std::string getXDGDataHomeEnv();

	// Gets the config home directory from $XDG_CONFIG_HOME (or $HOME/.config as a fallback).
	// Does not have a trailing slash. SDL does not supply a matching function for this (unlike
	// SDL_GetPrefPath()).
	static std::string getXDGConfigHomeEnv();
public:
	// Gets the current platform name via SDL_GetPlatform().
	static std::string getPlatform();

	// Gets the base path to the executable (mostly intended for macOS .app).
	static std::string getBasePath();

	// Gets the options folder path.
	static std::string getOptionsPath();

	// Gets the screenshot folder path via SDL_GetPrefPath().
	static std::string getScreenshotPath();

	// Gets the log folder path for logging program messages.
	static std::string getLogPath();

	// Gets the max number of threads available on the CPU.
	static int getThreadCount();
};

#endif
