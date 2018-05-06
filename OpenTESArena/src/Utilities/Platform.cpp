#include <thread>

#include "SDL.h"

#include "Debug.h"
#include "Platform.h"
#include "String.h"

#if defined(_WINDOWS)
#include <Windows.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#endif

const std::string Platform::XDGDataHome = "XDG_DATA_HOME";
const std::string Platform::XDGConfigHome = "XDG_CONFIG_HOME";

std::string Platform::getHomeEnv()
{
	const char *homeEnvPtr = SDL_getenv("HOME");
	return (homeEnvPtr != nullptr) ? std::string(homeEnvPtr) : std::string();
}

std::string Platform::getXDGDataHomeEnv()
{
	const char *xdgEnv = SDL_getenv(Platform::XDGDataHome.c_str());
	return (xdgEnv != nullptr) ? std::string(xdgEnv) :
		(Platform::getHomeEnv() + "/.local/share");
}

std::string Platform::getXDGConfigHomeEnv()
{
	const char *xdgEnv = SDL_getenv(Platform::XDGConfigHome.c_str());
	return (xdgEnv != nullptr) ? std::string(xdgEnv) :
		(Platform::getHomeEnv() + "/.config");
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
		DebugWarning("SDL_GetBasePath() not available on this platform.");
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
			DebugWarning("SDL_GetPrefPath() not available on this platform.");
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
		DebugWarning("No default options path on this platform.");
		return "OpenTESArena/options/";
	}
}

std::string Platform::getScreenshotPath()
{
	// SDL_GetPrefPath() creates the desired folder if it doesn't exist.
	char *screenshotPathPtr = SDL_GetPrefPath("OpenTESArena", "screenshots");

	if (screenshotPathPtr == nullptr)
	{
		DebugWarning("SDL_GetPrefPath() not available on this platform.");
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
			DebugWarning("SDL_GetPrefPath() not available on this platform.");
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
		DebugWarning("No default log path on this platform.");
		return "OpenTESArena/log/";
	}
}

int Platform::getThreadCount()
{
	const int threadCount = static_cast<int>(std::thread::hardware_concurrency());

	// hardware_concurrency() might return 0, so it needs to be clamped positive.
	if (threadCount == 0)
	{
		DebugWarning("hardware_concurrency() returned 0.");
		return 1;
	}
	else
	{
		return threadCount;
	}
}

bool Platform::directoryExists(const std::string &path)
{
#if defined(_WINDOWS)
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
	// Unknown platform.
	DebugNotImplemented();
	return false;
#endif
}

namespace
{
#if defined(_WINDOWS)
	void createWindowsDirectory(const std::string &path)
	{
		CreateDirectoryA(path.c_str(), nullptr);
	}
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	void createUnixDirectory(const std::string &path, mode_t permissions)
	{
		if (mkdir(path.c_str(), permissions) == -1)
		{
			DebugWarning("mkdir(): " + std::string(strerror(errno)) + ".");
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
#if defined(_WINDOWS)
			createWindowsDirectory(subStr);
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
			createUnixDirectory(subStr, 0700);
#else
			// Unknown platform.
			DebugNotImplemented();
#endif
		}
	} while (index != std::string::npos);
}
