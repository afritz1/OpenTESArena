#ifndef TEXT_MAP_H
#define TEXT_MAP_H

#include <cstdint>
#include <map>
#include <string>

// A KvpTextMap reads in a key-value pair file that uses the "key=value" syntax.
// The parser ignores lines where the first column is '#'.

// Pairs can be listed in the file in any order.

class KvpTextMap
{
	static const char COMMENT;

	std::map<std::string, std::string> pairs;

	// Use this method to access the pairs, since it does error checking.
	const std::string &getValue(const std::string &key) const;
public:
	// Convert key-value pairs in a file to string->string mappings.
	KvpTextMap(const std::string &filename);
	~KvpTextMap();

	bool getBoolean(const std::string &key) const;
	int32_t getInteger(const std::string &key) const;
	double getDouble(const std::string &key) const;
	std::string getString(const std::string &key) const;
};

#endif
