#ifndef ARENA_LEVEL_LIBRARY_H
#define ARENA_LEVEL_LIBRARY_H

#include "MIFFile.h"
#include "RMDFile.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Singleton.h"
#include "components/utilities/Span.h"

class ArenaLevelLibrary : public Singleton<ArenaLevelLibrary>
{
private:
	Buffer<MIFFile> cityBlockMifs;
	Buffer<RMDFile> wildernessChunks; // WILD001 to WILD070.

	bool initCityBlockMifs();
	bool initWildernessChunks();
public:
	bool init();

	Span<const MIFFile> getCityBlockMifs() const;
	Span<const RMDFile> getWildernessChunks() const;
};

#endif
