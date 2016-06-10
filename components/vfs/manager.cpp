
#include "manager.hpp"

#ifdef _WIN32
#include "dirent.h"
#include "fnmatch.h"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#endif

#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>


namespace
{

std::vector<std::string> gRootPaths;

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

    return IStreamPtr();
}

bool Manager::exists(const char *name)
{
    std::ifstream file;
    for(const std::string &path : gRootPaths)
    {
        file.open((path+name).c_str(), std::ios_base::binary);
        if(file.is_open()) return true;
    }

    return false;
}


void Manager::add_dir(const std::string &path, const std::string &pre, const char *pattern, std::set<std::string> &names)
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
                names.insert(fname);
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

std::set<std::string> Manager::list(const char *pattern) const
{
    std::set<std::string> files;

    auto piter = gRootPaths.rbegin();
    while(piter != gRootPaths.rend())
    {
        add_dir(*piter+".", "", pattern, files);
        ++piter;
    }

    return files;
}

} // namespace VFS
