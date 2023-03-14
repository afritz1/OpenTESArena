#include "Directory.h"

#include "../debug/Debug.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#endif

bool Directory::exists(const std::string &path)
{
#if defined(_WIN32)
	const DWORD attrs = GetFileAttributes(path.c_str());
	return (attrs != INVALID_FILE_ATTRIBUTES) && ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
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

void Directory::createRecursively(std::string path)
{
	const bool pathIsEmpty = path.size() == 0;
	const bool hasTrailingSlash = !pathIsEmpty && ((path.back() == '/') || (path.back() == '\\'));

	if (!hasTrailingSlash)
	{
		path.push_back('/');
	}

	size_t index = 0;
	do
	{
		index = path.find_first_of("\\/", index + 1);
		const std::string subStr = path.substr(0, index);

		if (!Directory::exists(subStr))
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