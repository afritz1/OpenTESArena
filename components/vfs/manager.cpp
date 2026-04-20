#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "manager.hpp"
#include "../archives/bsaarchive.hpp"
#include "../debug/Debug.h"
#include "../utilities/Directory.h"
#include "../utilities/Span.h"
#include "../utilities/StringView.h"

namespace
{
	std::vector<std::string> gRootPaths;
	Archives::BsaArchive gGlobalBsa;
}

VFS::Manager::Manager()
{
}

void VFS::Manager::initialize(std::string&& rootPath)
{
	if (rootPath.empty())
		rootPath += "./";
	else if ((rootPath.back() != '/') && (rootPath.back() != '\\'))
		rootPath += '/';

	gGlobalBsa.load(rootPath + "GLOBAL.BSA");
	gRootPaths.push_back(std::move(rootPath));
}

void VFS::Manager::addDataPath(std::string&& path)
{
	if (path.empty())
		path += "./";
	else if ((path.back() != '/') && (path.back() != '\\'))
		path += '/';

	gRootPaths.push_back(std::move(path));
}

VFS::IStreamPtr VFS::Manager::open(const char *name, bool *inGlobalBSA)
{
	DebugAssert(name != nullptr);
	DebugAssert(inGlobalBSA != nullptr);

	// Look for a root directory that contains the file, preferring newest roots.
	for (int i = static_cast<int>(gRootPaths.size()) - 1; i >= 0; i--)
	{
		const std::string &rootPath = gRootPaths[i];
		const std::string filePath = rootPath + name;

		std::error_code errorCode;
		if (std::filesystem::is_regular_file(filePath, errorCode))
		{
			*inGlobalBSA = false;

			std::unique_ptr<std::ifstream> stream = std::make_unique<std::ifstream>();
			stream->open(filePath, std::ios::binary);
			return VFS::IStreamPtr(std::move(stream));
		}
	}

	// Not a loose file, try GLOBAL.BSA instead.
	Archives::IStreamPtr bsaStream = gGlobalBsa.open(name);
	*inGlobalBSA = bsaStream != nullptr;
	return bsaStream;
}

VFS::IStreamPtr VFS::Manager::open(const char *name)
{
	bool dummy;
	return this->open(name, &dummy);
}

VFS::IStreamPtr VFS::Manager::openCaseInsensitive(const char *name, bool *inGlobalBSA)
{
	// Since the given filename is assumed to be unique in its directory, we only need to
	// worry about filenames just like it but with different casing.
	std::string newName = name;

	// Case 1: upper first character, lower rest.
	newName.front() = std::toupper(newName.front());
	std::for_each(newName.begin() + 1, newName.end(),
		[](char &c) { c = std::tolower(c); });

	IStreamPtr stream = this->open(newName.c_str(), inGlobalBSA);

	if (stream != nullptr)
	{
		return stream;
	}
	else
	{
		// Case 2: all uppercase.
		for (char &c : newName)
		{
			c = std::toupper(c);
		}

		stream = this->open(newName.c_str(), inGlobalBSA);

		// The caller does error checking to see if this is null.
		return stream;
	}
}

VFS::IStreamPtr VFS::Manager::openCaseInsensitive(const char *name)
{
	bool dummy;
	return this->openCaseInsensitive(name, &dummy);
}

bool VFS::Manager::read(const char *name, Buffer<std::byte> *dst, bool *inGlobalBSA)
{
	DebugAssert(name != nullptr);
	DebugAssert(dst != nullptr);

	VFS::IStreamPtr stream = this->open(name, inGlobalBSA);
	if (stream == nullptr)
	{
		DebugLogErrorFormat("Could not open \"%s\".", name);
		return false;
	}

	stream->seekg(0, std::ios::end);
	dst->init(static_cast<int>(stream->tellg()));
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(dst->begin()), dst->getCount());
	return true;
}

bool VFS::Manager::read(const char *name, Buffer<std::byte> *dst)
{
	bool dummy;
	return this->read(name, dst, &dummy);
}

bool VFS::Manager::readCaseInsensitive(const char *name, Buffer<std::byte> *dst, bool *inGlobalBSA)
{
	DebugAssert(name != nullptr);
	DebugAssert(dst != nullptr);

	VFS::IStreamPtr stream = this->openCaseInsensitive(name, inGlobalBSA);
	if (stream == nullptr)
	{
		DebugLogErrorFormat("Could not open \"%s\".", name);
		return false;
	}

	stream->seekg(0, std::ios::end);
	dst->init(static_cast<int>(stream->tellg()));
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(dst->begin()), dst->getCount());
	return true;
}

bool VFS::Manager::readCaseInsensitive(const char *name, Buffer<std::byte> *dst)
{
	bool dummy;
	return this->readCaseInsensitive(name, dst, &dummy);
}

bool VFS::Manager::exists(const char *name)
{
	for (int i = static_cast<int>(gRootPaths.size()) - 1; i >= 0; i--)
	{
		const std::string &rootPath = gRootPaths[i];
		const std::string filePath = rootPath + name;
		
		std::error_code errorCode;
		if (std::filesystem::exists(filePath, errorCode))
		{
			return true;
		}
	}

	return gGlobalBsa.exists(name);
}

std::vector<std::string> VFS::Manager::getFilesWithExtensionRecursively(const std::string &directoryPath, const char *extension)
{
	std::vector<std::string> filenames;
	if (!Directory::exists(directoryPath.c_str()))
	{
		return filenames;
	}

	const std::filesystem::path directoryFsPath(directoryPath);
	for (const std::filesystem::directory_entry &dirEntry : std::filesystem::recursive_directory_iterator(directoryFsPath))
	{
		std::error_code errorCode;
		if (!dirEntry.is_regular_file(errorCode))
		{
			continue;
		}

		const std::filesystem::path filePath = dirEntry.path();
		const std::string fileExtension = filePath.extension().string();
		if (!StringView::caseInsensitiveEquals(StringView::getExtension(fileExtension), extension))
		{
			continue;
		}

		const std::filesystem::path fileRelativePath = std::filesystem::relative(filePath, directoryFsPath);
		filenames.emplace_back(fileRelativePath.generic_string());
	}

	std::sort(filenames.begin(), filenames.end());
	return filenames;
}

std::vector<std::string> VFS::Manager::listFilesWithExtension(const char *extension) const
{
	std::vector<std::string> filenames;
	for (int i = static_cast<int>(gRootPaths.size()) - 1; i >= 0; i--)
	{
		const std::string &rootPath = gRootPaths[i];
		std::vector<std::string> filenamesInRootPath = VFS::Manager::getFilesWithExtensionRecursively(rootPath, extension);
		if (!filenamesInRootPath.empty())
		{
			filenames.insert(filenames.end(), filenamesInRootPath.begin(), filenamesInRootPath.end());
		}
	}

	Span<const std::string> bsaList = gGlobalBsa.list();
	for (const std::string &bsaEntryFilename : bsaList)
	{
		const std::string_view bsaEntryFileExtension = StringView::getExtension(bsaEntryFilename);
		if (!StringView::caseInsensitiveEquals(bsaEntryFileExtension, extension))
		{
			continue;
		}

		filenames.emplace_back(bsaEntryFilename);
	}

	return filenames;
}
