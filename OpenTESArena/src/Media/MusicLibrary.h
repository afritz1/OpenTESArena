#ifndef MUSIC_LIBRARY_H
#define MUSIC_LIBRARY_H

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "MusicDefinition.h"

#include "components/utilities/BufferView.h"

class Random;

class MusicLibrary
{
public:
	using Predicate = std::function<bool(const MusicDefinition&)>;
private:
	std::unordered_map<MusicDefinition::Type, std::vector<MusicDefinition>> definitions;

	static bool tryParseType(const std::string_view &typeStr, MusicDefinition::Type *outType);
	static bool tryParseValue(const std::string_view &valueStr, MusicDefinition::Type type,
		MusicDefinition *outDefinition);
public:
	// Parses music definition file.
	bool init(const char *filename);

	int getMusicDefinitionCount(MusicDefinition::Type type) const;
	const MusicDefinition *getMusicDefinition(MusicDefinition::Type type, int index) const;
	const MusicDefinition *getFirstMusicDefinition(MusicDefinition::Type type) const;
	const MusicDefinition *getRandomMusicDefinition(MusicDefinition::Type type, Random &random) const;
	const MusicDefinition *getRandomMusicDefinitionIf(MusicDefinition::Type type, Random &random,
		const Predicate &predicate) const;
};

#endif
