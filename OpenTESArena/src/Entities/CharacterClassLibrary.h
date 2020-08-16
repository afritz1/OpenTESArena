#ifndef CHARACTER_CLASS_LIBRARY_H
#define CHARACTER_CLASS_LIBRARY_H

#include <functional>
#include <string>
#include <vector>

#include "CharacterClassDefinition.h"

class ExeData;

class CharacterClassLibrary
{
public:
	using Predicate = std::function<bool(const CharacterClassDefinition&)>;
private:
	std::vector<CharacterClassDefinition> defs;
public:
	void init(const ExeData &exeData);

	int getDefinitionCount() const;
	bool findDefinitionIndexIf(const Predicate &predicate, int *outIndex) const;
	const CharacterClassDefinition &getDefinition(int index) const;
	bool tryGetDefinitionIndex(const CharacterClassDefinition &def, int *outIndex) const;
};

#endif
