#ifndef ARENA_LEVEL_LIBRARY_H
#define ARENA_LEVEL_LIBRARY_H

#include "MIFFile.h"
#include "RMDFile.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/Singleton.h"

class ArenaLevelLibrary : public Singleton<ArenaLevelLibrary>
{
private:
	Buffer<MIFFile> cityBlockMifs;
	Buffer<RMDFile> wildernessChunks; // WILD001 to WILD070.

	bool initCityBlockMifs();
	bool initWildernessChunks();
public:
	bool init();

	BufferView<const MIFFile> getCityBlockMifs() const;
	BufferView<const RMDFile> getWildernessChunks() const;
};

#endif
