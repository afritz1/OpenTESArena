
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
    if(root_path.empty())
        root_path += "./";
    else if(root_path.back() != '/' && root_path.back() != '\\')
        root_path += "/";

    gGlobalBsa.load(root_path+"GLOBAL.BSA");

    gRootPaths.push_back(std::move(root_path));
}

void Manager::addDataPath(std::string&& path)
{
    if(path.empty())
        path += "./";
    else if(path.back() != '/' && path.back() != '\\')
        path += "/";
    gRootPaths.push_back(std::move(path));
}


IStreamPtr Manager::open(const char *name)
{
    std::unique_ptr<std::ifstream> stream(new std::ifstream());
    // Search in reverse, so newer paths take precedence.
    auto piter = gRootPaths.rbegin();
    while(piter != gRootPaths.rend())
    {
        stream->open((*piter+name).c_str(), std::ios_base::binary);
        if(stream->good()) return IStreamPtr(std::move(stream));
        ++piter;
    }

    return gGlobalBsa.open(name);
}

bool Manager::exists(const char *name)
{
    std::ifstream file;
    for(const std::string &path : gRootPaths)
    {
        file.open((path+name).c_str(), std::ios_base::binary);
        if(file.is_open()) return true;
    }

    return gGlobalBsa.exists(name);
}


void Manager::add_dir(const std::string &path, const std::string &pre, const char *pattern, std::vector<std::string> &names)
{
    DIR *dir = opendir(path.c_str());
    if(!dir) return;

    dirent *ent;
    while((ent=readdir(dir)) != nullptr)
    {
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        if(!S_ISDIR(ent->d_type))
        {
            std::string fname = pre + ent->d_name;
            if(!pattern || fnmatch(pattern, fname.c_str(), 0) == 0)
                names.push_back(fname);
        }
        else
        {
            std::string newpath = path+"/"+ent->d_name;
            std::string newpre = pre+ent->d_name+"/";
            add_dir(newpath, newpre, pattern, names);
        }
    }

    closedir(dir);
}

std::vector<std::string> Manager::list(const char *pattern) const
{
    std::vector<std::string> files;

    auto piter = gRootPaths.rbegin();
    while(piter != gRootPaths.rend())
    {
        add_dir(*piter+".", "", pattern, files);
        ++piter;
    }

    if(!pattern)
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
                return (fnmatch(pattern, dirsep?dirsep:name.c_str(), 0) == 0);
            }
        );
    }

    return files;
}

} // namespace VFS
