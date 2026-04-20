#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../utilities/Buffer.h"

namespace VFS
{

using IStreamPtr = std::shared_ptr<std::istream>;

class Manager
{
	Manager(const Manager&) = delete;
	Manager& operator=(const Manager&) = delete;

	static std::vector<std::string> getFilesWithExtensionRecursively(const std::string &directoryPath, const char *extension);

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
	std::vector<std::string> listFilesWithExtension(const char *extension) const;

	static Manager &get()
	{
		static Manager manager;
		return manager;
	}
};

} // namespace VFS
