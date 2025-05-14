#ifndef CHARACTER_CLASS_LIBRARY_H
#define CHARACTER_CLASS_LIBRARY_H

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
	bool findDefinitionIndexIf(const Predicate &predicate, int *outIndex) const;
	bool tryGetDefinitionIndex(const CharacterClassDefinition &def, int *outIndex) const;
};

#endif
