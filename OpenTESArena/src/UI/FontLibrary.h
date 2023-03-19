#ifndef FONT_LIBRARY_H
#define FONT_LIBRARY_H

#include <string>
#include <vector>

#include "FontDefinition.h"

#include "components/utilities/Singleton.h"

class FontLibrary : public Singleton<FontLibrary>
{
private:
	std::vector<FontDefinition> defs;
public:
	bool init();

	int getDefinitionCount() const;
	bool tryGetDefinitionIndex(const char *name, int *outIndex) const; // @todo: change to std::optional
	const FontDefinition &getDefinition(int index) const;
};

#endif
