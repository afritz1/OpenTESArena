#ifndef KEY_VALUE_FILE_H
#define KEY_VALUE_FILE_H

#include <string>
#include <string_view>
#include <unordered_map>

// A key-value map reads in a key-value pair file that uses the "key = value" syntax.
// Pairs are associated with a section and can be listed in the file in any order.
// Comments can be anywhere in a line.

class KeyValueFile
{
public:
	typedef std::unordered_map<std::string, std::string> SectionMap;
private:
	// Each section has a map of key-value pairs.
	std::unordered_map<std::string, SectionMap> sectionMaps;

	// Use this function to access the section maps since it does error checking.
	bool tryGetValue(const std::string &section, const std::string &key, std::string_view &value) const;
public:
	// These are public so other code can use them (i.e., for options writing).
	static const char COMMENT;
	static const char PAIR_SEPARATOR;
	static const char SECTION_FRONT;
	static const char SECTION_BACK;

	bool init(const char *filename);

	// Typed getter methods for convenience.
	bool tryGetBoolean(const std::string &section, const std::string &key, bool &value) const;
	bool tryGetInteger(const std::string &section, const std::string &key, int &value) const;
	bool tryGetDouble(const std::string &section, const std::string &key, double &value) const;
	bool tryGetString(const std::string &section, const std::string &key, std::string_view &value) const;

	// Gets a reference to all section maps. Intended for iteration.
	const std::unordered_map<std::string, SectionMap> &getAll() const;
};

#endif
