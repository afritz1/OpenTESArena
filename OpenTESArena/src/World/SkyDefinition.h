#ifndef SKY_DEFINITION_H
#define SKY_DEFINITION_H

#include <vector>

#include "SkyObjectDefinition.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

// Contains a location's distant sky values and objects (mountains, clouds, stars, etc.).

class SkyDefinition
{
private:
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
