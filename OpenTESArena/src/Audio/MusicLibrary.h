#ifndef MUSIC_LIBRARY_H
#define MUSIC_LIBRARY_H

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "MusicDefinition.h"

#include "components/utilities/BufferView.h"
#include "components/utilities/Singleton.h"

class Random;

class MusicLibrary : public Singleton<MusicLibrary>
{
public:
	using Predicate = std::function<bool(const MusicDefinition&)>;
private:
	std::unordered_map<MusicType, std::vector<MusicDefinition>> definitions;

	static bool tryParseType(const std::string_view &typeStr, MusicType *outType);
	static bool tryParseValue(const std::string_view &valueStr, MusicType type, MusicDefinition *outDefinition);
public:
	// Parses music definition file.
	bool init(const char *filename);

	int getMusicDefinitionCount(MusicType type) const;
	const MusicDefinition *getMusicDefinition(MusicType type, int index) const;
	const MusicDefinition *getFirstMusicDefinition(MusicType type) const;
	const MusicDefinition *getRandomMusicDefinition(MusicType type, Random &random) const;
	const MusicDefinition *getRandomMusicDefinitionIf(MusicType type, Random &random, const Predicate &predicate) const;
};

#endif
