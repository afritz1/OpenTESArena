#ifndef CHARACTER_RACE_LIBRARY_H
#define CHARACTER_RACE_LIBRARY_H

#include <functional>
#include <vector>

#include "CharacterRaceDefinition.h"

#include "components/utilities/Singleton.h"

struct ExeData;

class CharacterRaceLibrary : public Singleton<CharacterRaceLibrary>
{
public:
	using Predicate = std::function<bool(const CharacterRaceDefinition&)>;
private:
	std::vector<CharacterRaceDefinition> defs;
public:
	void init(const ExeData &exeData);

	int getDefinitionCount() const;
	const CharacterRaceDefinition &getDefinition(int index) const;
	bool findDefinitionIndexIf(const Predicate &predicate, int *outIndex) const;
	bool tryGetDefinitionIndex(const CharacterRaceDefinition &def, int *outIndex) const;
};

#endif
