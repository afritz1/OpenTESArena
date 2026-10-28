#pragma once

#include <functional>
#include <string>
#include <vector>

#include "CharacterClassDefinition.h"

#include "components/utilities/Singleton.h"

struct ExeData;

using CharacterClassDefinitionID = int;
using CharacterClassPredicate = std::function<bool(const CharacterClassDefinition&)>;

class CharacterClassLibrary : public Singleton<CharacterClassLibrary>
{
private:
	std::vector<CharacterClassDefinition> defs;
public:
	void init(const ExeData &exeData);

	int getDefinitionCount() const;
	const CharacterClassDefinition &getDefinition(CharacterClassDefinitionID id) const;
	CharacterClassDefinitionID findDefinitionIdIf(const CharacterClassPredicate &predicate) const;
	CharacterClassDefinitionID tryGetDefinitionID(const CharacterClassDefinition &def) const;
};
