#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "MusicDefinition.h"

#include "components/utilities/Singleton.h"
#include "components/utilities/Span.h"

class Random;

using MusicDefinitionPredicate = std::function<bool(const MusicDefinition&)>;

class MusicLibrary : public Singleton<MusicLibrary>
{
private:
	std::unordered_map<MusicType, std::vector<MusicDefinition>> definitions;

	static bool tryParseValue(const std::string_view valueStr, MusicType type, MusicDefinition *outDefinition);
public:
	// Parses music definition file.
	bool init(const char *filename);

	int getMusicDefinitionCount(MusicType type) const;
	const MusicDefinition *getMusicDefinition(MusicType type, int index) const;
	const MusicDefinition *getFirstMusicDefinition(MusicType type) const;
	const MusicDefinition *getRandomMusicDefinition(MusicType type, Random &random) const;
	const MusicDefinition *getRandomMusicDefinitionIf(MusicType type, Random &random, const MusicDefinitionPredicate &predicate) const;
};
