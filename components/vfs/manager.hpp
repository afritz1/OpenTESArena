#ifndef COMPONENTS_VFS_MANAGER_HPP
#define COMPONENTS_VFS_MANAGER_HPP

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace VFS
{

typedef std::shared_ptr<std::istream> IStreamPtr;

inline uint32_t read_le32(std::istream &stream)
{
	std::array<char, 4> buf;
	if (!stream.read(buf.data(), buf.size()) || stream.gcount() != buf.size())
		return 0;
	return ((uint32_t(buf[0]) & 0x000000ff) | (uint32_t(buf[1] << 8) & 0x0000ff00) |
		(uint32_t(buf[2] << 16) & 0x00ff0000) | (uint32_t(buf[3] << 24) & 0xff000000));
}

inline uint16_t read_le16(std::istream &stream)
{
	std::array<char, 2> buf;
	if (!stream.read(buf.data(), buf.size()) || stream.gcount() != buf.size())
		return 0;
	return ((uint16_t(buf[0]) & 0x00ff) | (uint16_t(buf[1] << 8) & 0xff00));
}

class Manager {
	Manager(const Manager&) = delete;
	Manager& operator=(const Manager&) = delete;

	static void addDir(const std::string &path, const std::string &pre, const char *pattern,
		std::vector<std::string> &names);

	Manager();

public:
	void initialize(std::string&& rootPath = std::string());
	void addDataPath(std::string&& path);

	IStreamPtr open(const char *name, bool &inGlobalBSA);
	IStreamPtr open(const char *name);
	IStreamPtr open(const std::string &name, bool &inGlobalBSA);
	IStreamPtr open(const std::string &name);

	// Special open method intended for Unix systems since the Arena floppy and CD versions don't
	// have consistent casing for some files (like SPELLSG.65). This method is specific to Arena's
	// files and is not a general solution for case-insensitive file loading.
	// - To do: replace with something using std::filesystem::equivalent() (C++17)?
	IStreamPtr openCaseInsensitive(const std::string &name, bool &inGlobalBSA);
	IStreamPtr openCaseInsensitive(const std::string &name);

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
