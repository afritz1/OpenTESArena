#ifndef STEAM_KEY_VALUE_FILE_H
#define STEAM_KEY_VALUE_FILE_H

#include <string>
#include <unordered_map>

struct SteamKeyValueEntry
{
	std::string name;
	std::unordered_map<std::string, std::string> valuePairs;
	std::unordered_map<std::string, SteamKeyValueEntry> entryPairs;
};

// Parses .vdf and .acf.
struct SteamKeyValueFile
{
	SteamKeyValueEntry root;

	bool init(const char *filename);
};

#endif
