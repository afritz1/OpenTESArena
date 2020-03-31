#include <thread>

#include "SDL.h"

#include "Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace Platform
{
	// Linux user environment variables. Data home is "~/.local/share" and config home is
	// "~/.config". If getenv("XDG_...") is null, then try getenv("HOME") with the desired
	// subdirectory appended (i.e., "./local/share").
	const std::string XDGDataHome = "XDG_DATA_HOME";
	const std::string XDGConfigHome = "XDG_CONFIG_HOME";

	// Gets the user's home environment variable ($HOME). Does not have a trailing slash.
	std::string getHomeEnv()
	{
		const char *homeEnvPtr = SDL_getenv("HOME");
		return (homeEnvPtr != nullptr) ? std::string(homeEnvPtr) : std::string();
	}

	// Gets the data home directory from $XDG_DATA_HOME (or $HOME/.local/share as a fallback).
	// Does not have a trailing slash. Basically the same as SDL_GetPrefPath() on Linux systems
	// (without the slash).
	std::string getXDGDataHomeEnv()
	{
		const char *xdgEnv = SDL_getenv(Platform::XDGDataHome.c_str());
		return (xdgEnv != nullptr) ? std::string(xdgEnv) :
			(Platform::getHomeEnv() + "/.local/share");
	}

	// Gets the config home directory from $XDG_CONFIG_HOME (or $HOME/.config as a fallback).
	// Does not have a trailing slash. SDL does not supply a matching function for this (unlike
	// SDL_GetPrefPath()).
	std::string getXDGConfigHomeEnv()
	{
		const char *xdgEnv = SDL_getenv(Platform::XDGConfigHome.c_str());
		return (xdgEnv != nullptr) ? std::string(xdgEnv) :
			(Platform::getHomeEnv() + "/.config");
	}
}

std::string Platform::getPlatform()
{
	return std::string(SDL_GetPlatform());
}

std::string Platform::getBasePath()
{
	// Allocate the base path from SDL.
	char *basePathPtr = SDL_GetBasePath();

	if (basePathPtr == nullptr)
	{
		DebugLogWarning("SDL_GetBasePath() not available on this platform.");
		basePathPtr = SDL_strdup("./");
	}

	const std::string basePathString(basePathPtr);
	SDL_free(basePathPtr);

	// Convert Windows backslashes to forward slashes.
	return String::replace(basePathString, '\\', '/');
}

std::string Platform::getOptionsPath()
{
	const std::string platform = Platform::getPlatform();

	if (platform == "Windows")
	{
		// SDL_GetPrefPath() creates the desired folder if it doesn't exist.
		char *optionsPathPtr = SDL_GetPrefPath("OpenTESArena", "options");

		if (optionsPathPtr == nullptr)
		{
			DebugLogWarning("SDL_GetPrefPath() not available on this platform.");
			optionsPathPtr = SDL_strdup("options/");
		}

		const std::string optionsPathString(optionsPathPtr);
		SDL_free(optionsPathPtr);

		// Convert Windows backslashes to forward slashes.
		return String::replace(optionsPathString, '\\', '/');
	}
	else if (platform == "Linux")
	{
		return Platform::getXDGConfigHomeEnv() + "/OpenTESArena/options/";
	}
	else if (platform == "Mac OS X")
	{
		return Platform::getHomeEnv() + "/Library/Preferences/OpenTESArena/options/";
	}
	else
	{
		DebugLogWarning("No default options path on this platform.");
		return "OpenTESArena/options/";
	}
}

std::string Platform::getScreenshotPath()
{
	// SDL_GetPrefPath() creates the desired folder if it doesn't exist.
	char *screenshotPathPtr = SDL_GetPrefPath("OpenTESArena", "screenshots");

	if (screenshotPathPtr == nullptr)
	{
		DebugLogWarning("SDL_GetPrefPath() not available on this platform.");
		screenshotPathPtr = SDL_strdup("screenshots/");
	}

	const std::string screenshotPathString(screenshotPathPtr);
	SDL_free(screenshotPathPtr);

	// Convert Windows backslashes to forward slashes.
	return String::replace(screenshotPathString, '\\', '/');
}

std::string Platform::getLogPath()
{
	// Unfortunately there's no SDL_GetLogPath(), so we need to make our own.
	const std::string platform = Platform::getPlatform();

	if (platform == "Windows")
	{
		char *logPathPtr = SDL_GetPrefPath("OpenTESArena", "log");

		if (logPathPtr == nullptr)
		{
			DebugLogWarning("SDL_GetPrefPath() not available on this platform.");
			logPathPtr = SDL_strdup("log/");
		}

		const std::string logPathString(logPathPtr);
		SDL_free(logPathPtr);

		// Convert Windows backslashes to forward slashes.
		return String::replace(logPathString, '\\', '/');
	}
	else if (platform == "Linux")
	{
		return Platform::getXDGConfigHomeEnv() + "/OpenTESArena/log/";
	}
	else if (platform == "Mac OS X")
	{
		return Platform::getHomeEnv() + "/Library/Logs/OpenTESArena/log/";
	}
	else
	{
		DebugLogWarning("No default log path on this platform.");
		return "OpenTESArena/log/";
	}
}

double Platform::getDefaultDPI()
{
	const std::string platform = Platform::getPlatform();

	// @todo: not sure how to handle Linux. Might need an ifdef and some environment variables.

	if (platform == "Windows")
	{
		return 96.0;
	}
	else if (platform == "Mac OS X")
	{
		return 72.0;
	}
	else
	{
		DebugLogWarning("No default DPI on this platform.");
		return 96.0;
	}
}

int Platform::getThreadCount()
{
	const int threadCount = static_cast<int>(std::thread::hardware_concurrency());

	// hardware_concurrency() might return 0, so it needs to be clamped positive.
	if (threadCount == 0)
	{
		DebugLogWarning("hardware_concurrency() returned 0.");
		return 1;
	}
	else
	{
		return threadCount;
	}
}

int Platform::getCacheLineSize()
{
	return SDL_GetCPUCacheLineSize();
}

bool Platform::hasSSE()
{
	return SDL_HasSSE() && SDL_HasSSE2() && SDL_HasSSE3() && SDL_HasSSE41() && SDL_HasSSE42();
}

bool Platform::hasAVX()
{
	return SDL_HasAVX() && SDL_HasAVX2();
}

bool Platform::directoryExists(const std::string &path)
{
#if defined(_WIN32)
	const DWORD attrs = GetFileAttributes(path.c_str());
	return (attrs != INVALID_FILE_ATTRIBUTES) &&
		((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	struct stat st;
	std::memset(&st, 0, sizeof(st));
	if (stat(path.c_str(), &st) == 0)
	{
		// Returns true if the entry is a directory.
		return (st.st_mode & S_IFDIR) != 0;
	}
	else
	{
		if (errno != ENOENT)
		{
			throw DebugException("stat(): " + std::string(strerror(errno)) + ".");
		}

		return false;
	}
#else
#error Unknown platform.
#endif
}

namespace
{
#if defined(_WIN32)
	void createWindowsDirectory(const std::string &path)
	{
		const BOOL success = CreateDirectoryA(path.c_str(), nullptr);
		if (success == 0)
		{
			const std::string message = [&path]() -> std::string
			{
				const DWORD lastError = GetLastError();
				if (lastError == ERROR_ALREADY_EXISTS)
				{
					return "\"" + path + "\" already exists.";
				}
				else if (lastError == ERROR_PATH_NOT_FOUND)
				{
					return "\"" + path + "\" not found.";
				}
				else
				{
					return "Unknown error.";
				}
			}();

			DebugLogWarning("CreateDirectoryA(): " + message);
		}
	}
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	void createUnixDirectory(const std::string &path, mode_t permissions)
	{
		if (mkdir(path.c_str(), permissions) == -1)
		{
			DebugLogWarning("mkdir(): " + std::string(strerror(errno)) + ".");
		}
	}
#endif
}

void Platform::createDirectoryRecursively(std::string path)
{
	const bool pathIsEmpty = path.size() == 0;
	const bool hasTrailingSlash = !pathIsEmpty &&
		((path.back() == '/') || (path.back() == '\\'));

	if (!hasTrailingSlash)
	{
		path.push_back('/');
	}

	size_t index = 0;
	do
	{
		index = path.find_first_of("\\/", index + 1);
		const std::string subStr = path.substr(0, index);

		if (!Platform::directoryExists(subStr))
		{
#if defined(_WIN32)
			createWindowsDirectory(subStr);
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
			createUnixDirectory(subStr, 0700);
#else
#error Unknown platform.
#endif
		}
	} while (index != std::string::npos);
}
