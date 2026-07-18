#include <filesystem>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include "SDL.h"

#include "Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/File.h"
#include "components/utilities/String.h"

namespace Platform
{
	// Linux user environment variables. Data home is "~/.local/share" and config home is
	// "~/.config". If getenv("XDG_...") is null, then try getenv("HOME") with the desired
	// subdirectory appended (i.e., "./local/share").
	const std::string XDGDataHome = "XDG_DATA_HOME";
	const std::string XDGConfigHome = "XDG_CONFIG_HOME";

	// Valve's app ID for The Elder Scrolls: Arena, used to find its manifest file inside a
	// Steam library's "steamapps" folder.
	const std::string SteamArenaAppID = "1812290";

	// GOG's product ID for The Elder Scrolls: Arena, used to find its install path in the
	// Windows registry.
	const std::string GogArenaProductID = "1435828982";

	// Gets the user's home environment variable ($HOME). Does not have a trailing slash.
	std::string getHomeEnv()
	{
		const char *homeEnvPtr = SDL_getenv("HOME");
		if (homeEnvPtr != nullptr)
		{
			return homeEnvPtr;
		}

		return std::string();
	}

	// Gets the data home directory from $XDG_DATA_HOME (or $HOME/.local/share as a fallback).
	// Does not have a trailing slash. Basically the same as SDL_GetPrefPath() on Linux systems
	// (without the slash).
	std::string getXDGDataHomeEnv()
	{
		const char *xdgEnv = SDL_getenv(Platform::XDGDataHome.c_str());
		if (xdgEnv != nullptr)
		{
			return xdgEnv;
		}

		return Platform::getHomeEnv() + "/.local/share";
	}

	// Gets the config home directory from $XDG_CONFIG_HOME (or $HOME/.config as a fallback).
	// Does not have a trailing slash. SDL does not supply a matching function for this (unlike
	// SDL_GetPrefPath()).
	std::string getXDGConfigHomeEnv()
	{
		const char *xdgEnv = SDL_getenv(Platform::XDGConfigHome.c_str());
		if (xdgEnv != nullptr)
		{
			return xdgEnv;			
		}

		return Platform::getHomeEnv() + "/.config";
	}

	// Gets the local Steam client's possible root install folders. On Linux there are several,
	// since Steam might be installed natively, via Flatpak, or via Snap.
	std::vector<std::string> getSteamRootCandidates()
	{
		const std::string platform = Platform::getPlatform();
		std::vector<std::string> candidates;

		if (platform == Platform::Linux)
		{
			const std::string home = Platform::getHomeEnv();
			candidates.emplace_back(home + "/.steam/steam");
			candidates.emplace_back(home + "/.steam/root");
			candidates.emplace_back(home + "/.local/share/Steam");
			candidates.emplace_back(home + "/.var/app/com.valvesoftware.Steam/.local/share/Steam");
			candidates.emplace_back(home + "/snap/steam/common/.local/share/Steam");
		}
		else if (platform == Platform::macOS)
		{
			candidates.emplace_back(Platform::getHomeEnv() + "/Library/Application Support/Steam");
		}
		else if (platform == Platform::Windows)
		{
#ifdef _WIN32
			char steamPathBuffer[MAX_PATH];
			DWORD steamPathSize = sizeof(steamPathBuffer);
			const LSTATUS status = RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath",
				RRF_RT_REG_SZ, nullptr, steamPathBuffer, &steamPathSize);
			if (status == ERROR_SUCCESS)
			{
				candidates.emplace_back(String::replace(std::string(steamPathBuffer), '\\', '/'));
			}
#endif
			candidates.emplace_back("C:/Program Files (x86)/Steam");
		}

		return candidates;
	}

	// Scans a Valve KeyValues (.vdf) file's text for every quoted value following a "path" key.
	// This intentionally isn't a general KeyValues parser -- libraryfolders.vdf only needs this
	// one key read out of it.
	std::vector<std::string> parseVdfLibraryPaths(const std::string &vdfText)
	{
		std::vector<std::string> paths;
		const std::string key = "\"path\"";

		size_t searchPos = 0;
		while (true)
		{
			const size_t keyPos = vdfText.find(key, searchPos);
			if (keyPos == std::string::npos)
			{
				break;
			}

			const size_t valueBegin = vdfText.find('"', keyPos + key.size());
			if (valueBegin == std::string::npos)
			{
				break;
			}

			const size_t valueEnd = vdfText.find('"', valueBegin + 1);
			if (valueEnd == std::string::npos)
			{
				break;
			}

			const std::string rawPath = vdfText.substr(valueBegin + 1, valueEnd - valueBegin - 1);
			paths.emplace_back(String::replace(rawPath, "\\\\", "/"));
			searchPos = valueEnd + 1;
		}

		return paths;
	}

	// Scans a Valve KeyValues (.acf) file's text for the first quoted value following the given
	// key. Used to read a single scalar field like "installdir" out of an appmanifest file.
	bool tryGetVdfValue(const std::string &vdfText, const std::string &key, std::string *outValue)
	{
		const std::string quotedKey = "\"" + key + "\"";
		const size_t keyPos = vdfText.find(quotedKey);
		if (keyPos == std::string::npos)
		{
			return false;
		}

		const size_t valueBegin = vdfText.find('"', keyPos + quotedKey.size());
		if (valueBegin == std::string::npos)
		{
			return false;
		}

		const size_t valueEnd = vdfText.find('"', valueBegin + 1);
		if (valueEnd == std::string::npos)
		{
			return false;
		}

		*outValue = vdfText.substr(valueBegin + 1, valueEnd - valueBegin - 1);
		return true;
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

std::vector<std::string> Platform::getSteamArenaPaths()
{
	std::vector<std::string> arenaPaths;

	for (const std::string &steamRoot : Platform::getSteamRootCandidates())
	{
		std::error_code dummy;
		if (!std::filesystem::is_directory(steamRoot, dummy))
		{
			continue;
		}

		// The Steam root is always library "0", included here directly in case the libraries
		// file below is missing or fails to parse.
		std::vector<std::string> libraryPaths = { steamRoot };

		const std::string vdfPath = steamRoot + "/steamapps/libraryfolders.vdf";
		if (File::exists(vdfPath.c_str()))
		{
			const std::string vdfText = File::readAllText(vdfPath.c_str());
			const std::vector<std::string> vdfLibraryPaths = Platform::parseVdfLibraryPaths(vdfText);
			libraryPaths.insert(libraryPaths.end(), vdfLibraryPaths.begin(), vdfLibraryPaths.end());
		}

		for (const std::string &libraryPath : libraryPaths)
		{
			const std::string acfPath = libraryPath + "/steamapps/appmanifest_" + Platform::SteamArenaAppID + ".acf";
			if (!File::exists(acfPath.c_str()))
			{
				continue;
			}

			const std::string acfText = File::readAllText(acfPath.c_str());
			std::string installDir;
			if (Platform::tryGetVdfValue(acfText, "installdir", &installDir))
			{
				arenaPaths.emplace_back(libraryPath + "/steamapps/common/" + installDir + "/ARENA");
			}
		}
	}

	return arenaPaths;
}

std::vector<std::string> Platform::getGogArenaPaths()
{
	std::vector<std::string> arenaPaths;

#ifdef _WIN32
	// GOG Arena is Windows-only and always writes its game install
	// path under WOW6432Node (even on win64). It is checked explicitly by name
	// first since Windows exempts direct access to it from the usual 32-bit/64-bit registry
	// redirection, which should keep this working independently of a 32 or 64-bit build. 
	// The non-redirected path is kept as a fallback in case that ever changes.
	const std::string candidateSubKeys[] =
	{
		"SOFTWARE\\WOW6432Node\\GOG.com\\Games\\" + Platform::GogArenaProductID,
		"SOFTWARE\\GOG.com\\Games\\" + Platform::GogArenaProductID
	};

	for (const std::string &subKey : candidateSubKeys)
	{
		char installPathBuffer[MAX_PATH];
		DWORD installPathSize = sizeof(installPathBuffer);
		const LSTATUS status = RegGetValueA(HKEY_LOCAL_MACHINE, subKey.c_str(), "path",
			RRF_RT_REG_SZ, nullptr, installPathBuffer, &installPathSize);
		if (status == ERROR_SUCCESS)
		{
			const std::string installPath = String::replace(std::string(installPathBuffer), '\\', '/');

			// Unlike the Steam release, the GOG release keeps Arena's data directly in the
			// install root (no "ARENA" subfolder).
			arenaPaths.emplace_back(installPath);
			break;
		}
	}
#endif

	return arenaPaths;
}

std::string Platform::getOptionsPath()
{
	const std::string platform = Platform::getPlatform();

	if (platform == Platform::Windows)
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
	else if (platform == Platform::Linux)
	{
		return Platform::getXDGConfigHomeEnv() + "/OpenTESArena/options/";
	}
	else if (platform == Platform::macOS)
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

	if (platform == Platform::Windows)
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
	else if (platform == Platform::Linux)
	{
		return Platform::getXDGConfigHomeEnv() + "/OpenTESArena/log/";
	}
	else if (platform == Platform::macOS)
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

	if (platform == Platform::Windows)
	{
		return 96.0;
	}
	else if (platform == Platform::macOS)
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
