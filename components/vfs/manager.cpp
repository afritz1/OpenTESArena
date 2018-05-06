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
#include <cctype>
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

void Manager::initialize(std::string&& root_path)
{
	if (root_path.empty())
		root_path += "./";
	else if (root_path.back() != '/' && root_path.back() != '\\')
		root_path += "/";

	gGlobalBsa.load(root_path + "GLOBAL.BSA");

	gRootPaths.push_back(std::move(root_path));
}

void Manager::addDataPath(std::string&& path)
{
	if (path.empty())
		path += "./";
	else if (path.back() != '/' && path.back() != '\\')
		path += "/";
	gRootPaths.push_back(std::move(path));
}

IStreamPtr Manager::open(const char *name, bool &inGlobalBSA)
{
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
		inGlobalBSA = false;
		return IStreamPtr(std::move(stream));
	}
	else
	{
		inGlobalBSA = true;
		return gGlobalBsa.open(name);
	}
}

IStreamPtr Manager::open(const char *name)
{
	bool dummy;
	return open(name, dummy);
}

IStreamPtr Manager::open(const std::string &name, bool &inGlobalBSA)
{
	return this->open(name.c_str(), inGlobalBSA);
}

IStreamPtr Manager::open(const std::string &name)
{
	bool dummy;
	return this->open(name, dummy);
}

IStreamPtr Manager::openCaseInsensitive(const std::string &name, bool &inGlobalBSA)
{
	// Since the given filename is assumed to be unique in its directory, we only need to
	// worry about filenames just like it but with different casing.
	std::string newName = name;

	// Case 1: upper first character, lower rest.
	newName.front() = std::toupper(newName.front());
	std::for_each(newName.begin() + 1, newName.end(),
		[](char &c) { c = std::tolower(c); });

	IStreamPtr stream = this->open(newName, inGlobalBSA);

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

		stream = this->open(newName, inGlobalBSA);

		// The caller does error checking to see if this is null.
		return stream;
	}
}

IStreamPtr Manager::openCaseInsensitive(const std::string &name)
{
	bool dummy;
	return this->openCaseInsensitive(name, dummy);
}

bool Manager::exists(const char *name)
{
	std::ifstream file;
	for (const std::string &path : gRootPaths)
	{
		file.open(path + name, std::ios_base::binary);
		if (file.is_open()) return true;
	}

	return gGlobalBsa.exists(name);
}

void Manager::add_dir(const std::string &path, const std::string &pre, const char *pattern, std::vector<std::string> &names)
{
	DIR *dir = opendir(path.c_str());
	if (!dir) return;

	dirent *ent;
	while ((ent = readdir(dir)) != nullptr)
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;

		if (!S_ISDIR(ent->d_type))
		{
			std::string fname = pre + ent->d_name;
			if (!pattern || fnmatch(pattern, fname.c_str(), 0) == 0)
				names.push_back(fname);
		}
		else
		{
			std::string newpath = path + "/" + ent->d_name;
			std::string newpre = pre + ent->d_name + "/";
			add_dir(newpath, newpre, pattern, names);
		}
	}

	closedir(dir);
}

std::vector<std::string> Manager::list(const char *pattern) const
{
	std::vector<std::string> files;

	auto piter = gRootPaths.rbegin();
	while (piter != gRootPaths.rend())
	{
		add_dir(*piter + ".", "", pattern, files);
		++piter;
	}

	if (!pattern)
	{
		const auto &list = gGlobalBsa.list();
		std::copy(list.begin(), list.end(), std::back_inserter(files));
	}
	else
	{
		const auto &list = gGlobalBsa.list();
		std::copy_if(list.begin(), list.end(), std::back_inserter(files),
			[pattern](const std::string &name) -> bool
		{
			const char *dirsep = strrchr(name.c_str(), '/');
			return (fnmatch(pattern, dirsep ? dirsep : name.c_str(), 0) == 0);
		});
	}

	return files;
}

} // namespace VFS
