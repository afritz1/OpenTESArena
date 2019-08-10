#ifndef KEY_VALUE_FILE_H
#define KEY_VALUE_FILE_H

#include <string>
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
	std::string filename;

	// Use this function to access the section maps since it does error checking.
	const std::string &getValue(const std::string &section, const std::string &key) const;
public:
	// These are public so other code can use them (i.e., for options writing).
	static const char COMMENT;
	static const char PAIR_SEPARATOR;
	static const char SECTION_FRONT;
	static const char SECTION_BACK;

	// Converts key-value pairs in a file to string->string mappings.
	KeyValueFile(const std::string &filename);

	// Typed getter methods for convenience.
	bool getBoolean(const std::string &section, const std::string &key) const;
	int getInteger(const std::string &section, const std::string &key) const;
	double getDouble(const std::string &section, const std::string &key) const;
	const std::string &getString(const std::string &section, const std::string &key) const;

	// Gets a reference to all section maps. Intended for iteration.
	const std::unordered_map<std::string, SectionMap> &getAll() const;
};

#endif
