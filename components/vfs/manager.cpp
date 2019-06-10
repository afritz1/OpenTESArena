#include "manager.hpp"

#ifdef _WIN32
#include "dirent.h"
#include "../misc/fnmatch.h"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#endif

#include <algorithm>
#include <cassert> // @todo: replace with DebugAssert
#include <cctype>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include "../archives/bsaarchive.hpp"

namespace
{
	std::vector<std::string> gRootPaths;
	Archives::BsaArchive gGlobalBsa;
}

namespace VFS
{

Manager::Manager()
{
}

void Manager::initialize(std::string&& rootPath)
{
	if (rootPath.empty())
		rootPath += "./";
	else if ((rootPath.back() != '/') && (rootPath.back() != '\\'))
		rootPath += '/';

	gGlobalBsa.load(rootPath + "GLOBAL.BSA");
	gRootPaths.push_back(std::move(rootPath));
}

void Manager::addDataPath(std::string&& path)
{
	if (path.empty())
		path += "./";
	else if ((path.back() != '/') && (path.back() != '\\'))
		path += '/';

	gRootPaths.push_back(std::move(path));
}

IStreamPtr Manager::open(const char *name, bool *inGlobalBSA)
{
	assert(name != nullptr);
	assert(inGlobalBSA != nullptr);

	std::unique_ptr<std::ifstream> stream(new std::ifstream());

	// Search in reverse, so newer paths take precedence.
	const auto iter = std::find_if(gRootPaths.rbegin(), gRootPaths.rend(),
		[name, &stream](const std::string &rootPath)
	{
		stream->open(rootPath + name, std::ios::binary);
		return stream->good();
	});

	if (iter != gRootPaths.rend())
	{
		*inGlobalBSA = false;
		return IStreamPtr(std::move(stream));
	}
	else
	{
		*inGlobalBSA = true;
		return gGlobalBsa.open(name);
	}
}

IStreamPtr Manager::open(const char *name)
{
	bool dummy;
	return this->open(name, &dummy);
}

IStreamPtr Manager::openCaseInsensitive(const char *name, bool *inGlobalBSA)
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

IStreamPtr Manager::openCaseInsensitive(const char *name)
{
	bool dummy;
	return this->openCaseInsensitive(name, &dummy);
}

bool Manager::read(const char *name, std::unique_ptr<std::byte[]> *dst, size_t *dstSize, bool *inGlobalBSA)
{
	assert(name != nullptr);
	assert(dst != nullptr);
	assert(dstSize != nullptr);

	IStreamPtr stream = this->open(name, inGlobalBSA);
	if (stream == nullptr)
	{
		// @todo: use Debug logging.
		std::cerr << "Could not open \"" << name << "\"." << '\n';
		return false;
	}

	stream->seekg(0, std::ios::end);
	*dstSize = stream->tellg();
	*dst = std::make_unique<std::byte[]>(*dstSize);
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(dst->get()), *dstSize);
	return true;
}

bool Manager::read(const char *name, std::unique_ptr<std::byte[]> *dst, size_t *dstSize)
{
	bool dummy;
	return this->read(name, dst, dstSize, &dummy);
}

bool Manager::readCaseInsensitive(const char *name, std::unique_ptr<std::byte[]> *dst, size_t *dstSize,
	bool *inGlobalBSA)
{
	assert(name != nullptr);
	assert(dst != nullptr);
	assert(dstSize != nullptr);

	IStreamPtr stream = this->openCaseInsensitive(name, inGlobalBSA);
	if (stream == nullptr)
	{
		// @todo: use Debug logging.
		std::cerr << "Could not open \"" << name << "\"." << '\n';
		return false;
	}

	stream->seekg(0, std::ios::end);
	*dstSize = stream->tellg();
	*dst = std::make_unique<std::byte[]>(*dstSize);
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(dst->get()), *dstSize);
	return true;
}

bool Manager::readCaseInsensitive(const char *name, std::unique_ptr<std::byte[]> *dst, size_t *dstSize)
{
	bool dummy;
	return this->readCaseInsensitive(name, dst, dstSize, &dummy);
}

bool Manager::exists(const char *name)
{
	std::ifstream file;
	const auto iter = std::find_if(gRootPaths.begin(), gRootPaths.end(),
		[name, &file](const std::string &rootPath)
	{
		file.open(rootPath + name, std::ios::binary);
		return file.is_open();
	});

	// If not in the root paths, then check inside GLOBAL.BSA.
	return (iter != gRootPaths.end()) || gGlobalBsa.exists(name);
}

void Manager::addDir(const std::string &path, const std::string &pre, const char *pattern,
	std::vector<std::string> &names)
{
	DIR *dir = opendir(path.c_str());
	if (dir == nullptr) return;

	dirent *ent;
	while ((ent = readdir(dir)) != nullptr)
	{
		if ((std::strcmp(ent->d_name, ".") == 0) || 
			(std::strcmp(ent->d_name, "..") == 0))
			continue;

		if (!S_ISDIR(ent->d_type))
		{
			std::string fname = pre + ent->d_name;
			if ((pattern == nullptr) || (fnmatch(pattern, fname.c_str(), 0) == 0))
				names.push_back(std::move(fname));
		}
		else
		{
			const std::string newPath = path + '/' + ent->d_name;
			const std::string newPre = pre + ent->d_name + '/';
			Manager::addDir(newPath, newPre, pattern, names);
		}
	}

	closedir(dir);
}

std::vector<std::string> Manager::list(const char *pattern) const
{
	std::vector<std::string> files;

	std::for_each(gRootPaths.rbegin(), gRootPaths.rend(),
		[pattern, &files](const std::string &rootPath)
	{
		Manager::addDir(rootPath + '.', std::string(), pattern, files);
	});

	const std::vector<std::string> &bsaList = gGlobalBsa.list();

	if (pattern == nullptr)
	{
		std::copy(bsaList.begin(), bsaList.end(), std::back_inserter(files));
	}
	else
	{
		std::copy_if(bsaList.begin(), bsaList.end(), std::back_inserter(files),
			[pattern](const std::string &name)
		{
			const char *dirsep = std::strrchr(name.c_str(), '/');
			return fnmatch(pattern, (dirsep != nullptr) ? dirsep : name.c_str(), 0) == 0;
		});
	}

	return files;
}

} // namespace VFS
