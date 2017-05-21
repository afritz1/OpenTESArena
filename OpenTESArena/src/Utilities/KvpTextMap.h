#ifndef TEXT_MAP_H
#define TEXT_MAP_H

#include <string>
#include <unordered_map>

// A KvpTextMap reads in a key-value pair file that uses the "key=value" syntax.
// The parser ignores lines where the first character is '#'.

// Pairs can be listed in the file in any order.

class KvpTextMap
{
	static const char COMMENT;

	std::unordered_map<std::string, std::string> pairs;
	std::string filename;

	// Use this method to access the pairs, since it does error checking.
	const std::string &getValue(const std::string &key) const;
public:
	// Convert key-value pairs in a file to string->string mappings.
	KvpTextMap(const std::string &filename);
	~KvpTextMap();

	bool getBoolean(const std::string &key) const;
	int getInteger(const std::string &key) const;
	double getDouble(const std::string &key) const;
	const std::string &getString(const std::string &key) const;
};

#endif
