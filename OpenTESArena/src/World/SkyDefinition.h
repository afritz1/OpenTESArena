#ifndef SKY_DEFINITION_H
#define SKY_DEFINITION_H

#include <vector>

#include "SkyObjectDefinition.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

// Contains a location's distant sky values and objects (mountains, clouds, stars, etc.).
// Similar to LevelDefinition where it defines where various sky objects will be once they
// are instanced.

class SkyDefinition
{
private:
	// @todo: want to change to be like LevelDefinition; land/air/star placement defs for
	// definitions stored in SkyInfoDefinition.

	std::vector<SkyObjectDefinition> objects;
	Buffer<Color> skyColors; // Colors for an entire day.
public:
	void init(Buffer<Color> &&skyColors);

	int getSkyColorCount() const;
	const Color &getSkyColor(int index);

	int getObjectCount() const;
	const SkyObjectDefinition &getObject(int index) const;

	int addObject(SkyObjectDefinition &&def);
};

#endif
