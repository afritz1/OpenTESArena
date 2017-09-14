#ifndef COMPONENTS_VFS_MANAGER_HPP
#define COMPONENTS_VFS_MANAGER_HPP

#include <string>
#include <iostream>
#include <memory>
#include <string>
#include <vector>


namespace VFS
{

typedef std::shared_ptr<std::istream> IStreamPtr;

inline uint32_t read_le32(std::istream &stream)
{
    char buf[4];
    if(!stream.read(buf, sizeof(buf)) || stream.gcount() != sizeof(buf))
        return 0;
    return ((uint32_t(buf[0]    )&0x000000ff) | (uint32_t(buf[1]<< 8)&0x0000ff00) |
            (uint32_t(buf[2]<<16)&0x00ff0000) | (uint32_t(buf[3]<<24)&0xff000000));
}

inline uint16_t read_le16(std::istream &stream)
{
    char buf[2];
    if(!stream.read(buf, sizeof(buf)) || stream.gcount() != sizeof(buf))
        return 0;
    return ((uint16_t(buf[0]   )&0x00ff) | (uint16_t(buf[1]<<8)&0xff00));
}


class Manager {
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    static void add_dir(const std::string &path, const std::string &pre, const char *pattern, std::vector<std::string> &names);

    Manager();

public:
    void initialize(std::string&& root_path=std::string());
    void addDataPath(std::string&& path);

    IStreamPtr open(const char *name);
    IStreamPtr open(const std::string &name) { return open(name.c_str()); }
    IStreamPtr open(std::string &&name) { return open(name.c_str()); }

    bool exists(const char *name);
    std::vector<std::string> list(const char *pattern=nullptr) const;

    static Manager &get()
    {
        static Manager manager;
        return manager;
    }
};

} // namespace VFS

#endif /* COMPONENTS_VFS_MANAGER_HPP */
