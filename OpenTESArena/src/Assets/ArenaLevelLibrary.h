#ifndef ARENA_LEVEL_LIBRARY_H
#define ARENA_LEVEL_LIBRARY_H

#include <vector>

#include "MIFFile.h"
#include "RMDFile.h"

#include "components/utilities/BufferView.h"
#include "components/utilities/Singleton.h"

class ArenaLevelLibrary : public Singleton<ArenaLevelLibrary>
{
private:
	std::vector<MIFFile> cityBlockMifs;
	std::vector<RMDFile> wildernessChunks; // WILD001 to WILD070.

	bool initCityBlockMifs();
	bool initWildernessChunks();
public:
	bool init();

	BufferView<const MIFFile> getCityBlockMifs() const;
	BufferView<const RMDFile> getWildernessChunks() const;
};

#endif
