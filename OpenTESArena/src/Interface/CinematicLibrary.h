#ifndef CINEMATIC_LIBRARY_H
#define CINEMATIC_LIBRARY_H

#include <functional>
#include <vector>

#include "TextCinematicDefinition.h"

class CinematicLibrary
{
public:
	using TextPredicate = std::function<bool(const TextCinematicDefinition&)>;
private:
	std::vector<TextCinematicDefinition> textDefs;
	// @todo: maybe store all different kinds of cinematics (new game, vision, etc.).
public:
	void init();

	int getTextDefinitionCount() const;
	const TextCinematicDefinition &getTextDefinition(int index) const;
	bool findTextDefinitionIndexIf(const TextPredicate &predicate, int *outIndex) const;
};

#endif
