#pragma once

#include <functional>
#include <string>
#include <vector>

#include "CharacterClassDefinition.h"

#include "components/utilities/Singleton.h"

struct ExeData;

class CharacterClassLibrary : public Singleton<CharacterClassLibrary>
{
public:
	using Predicate = std::function<bool(const CharacterClassDefinition&)>;
private:
	std::vector<CharacterClassDefinition> defs;
public:
	void init(const ExeData &exeData);

	int getDefinitionCount() const;
	const CharacterClassDefinition &getDefinition(int index) const;
	int findDefinitionIndexIf(const Predicate &predicate) const;
	bool tryGetDefinitionIndex(const CharacterClassDefinition &def, int *outIndex) const;
};
