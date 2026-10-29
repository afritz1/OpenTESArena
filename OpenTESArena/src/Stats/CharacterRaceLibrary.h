#pragma once

#include <functional>
#include <vector>

#include "CharacterRaceDefinition.h"

#include "components/utilities/Singleton.h"

class ProvinceLibrary;

struct ExeData;

using CharacterRaceDefinitionID = int;
using CharacterRacePredicate = std::function<bool(const CharacterRaceDefinition&)>;

class CharacterRaceLibrary : public Singleton<CharacterRaceLibrary>
{
private:
	std::vector<CharacterRaceDefinition> defs;
public:
	void init(const ProvinceLibrary &provinceLibrary, const ExeData &exeData);

	int getDefinitionCount() const;
	const CharacterRaceDefinition &getDefinition(CharacterRaceDefinitionID id) const;
	CharacterRaceDefinitionID findDefinitionIdIf(const CharacterRacePredicate &predicate) const;
};
