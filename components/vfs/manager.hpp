#ifndef COMPONENTS_VFS_MANAGER_HPP
#define COMPONENTS_VFS_MANAGER_HPP

#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../utilities/Buffer.h"

namespace VFS
{

typedef std::shared_ptr<std::istream> IStreamPtr;

class Manager {
	Manager(const Manager&) = delete;
	Manager& operator=(const Manager&) = delete;

	static void addDir(const std::string &path, const std::string &pre, const char *pattern,
		std::vector<std::string> &names);

	Manager();

public:
	void initialize(std::string&& rootPath = std::string());
	void addDataPath(std::string&& path);

	IStreamPtr open(const char *name, bool *inGlobalBSA);
	IStreamPtr open(const char *name);

	// Special open method intended for Unix systems since the Arena floppy and CD versions don't
	// have consistent casing for some files (like SPELLSG.65). This method is specific to Arena's
	// files and is not a general solution for case-insensitive file loading.
	// - To do: replace with something using std::filesystem::equivalent() (C++17)?
	IStreamPtr openCaseInsensitive(const char *name, bool *inGlobalBSA);
	IStreamPtr openCaseInsensitive(const char *name);

	// Convenience functions for opening and reading a file into the output parameters.
	bool read(const char *name, Buffer<std::byte> *dst, bool *inGlobalBSA);
	bool read(const char *name, Buffer<std::byte> *dst);
	bool readCaseInsensitive(const char *name, Buffer<std::byte> *dst, bool *inGlobalBSA);
	bool readCaseInsensitive(const char *name, Buffer<std::byte> *dst);

	bool exists(const char *name);
	std::vector<std::string> list(const char *pattern = nullptr) const;

	static Manager &get()
	{
		static Manager manager;
		return manager;
	}
};

} // namespace VFS

#endif /* COMPONENTS_VFS_MANAGER_HPP */
