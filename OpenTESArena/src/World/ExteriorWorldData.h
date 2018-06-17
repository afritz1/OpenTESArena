#ifndef EXTERIOR_WORLD_DATA_H
#define EXTERIOR_WORLD_DATA_H

#include <cstdint>
#include <vector>

#include "ExteriorLevelData.h"
#include "InteriorWorldData.h"
#include "LevelData.h"
#include "WorldData.h"
#include "../Assets/INFFile.h"
#include "../Math/Vector2.h"

class ExeData;
class MIFFile;
class MiscAssets;

enum class ClimateType;
enum class WeatherType;

class ExteriorWorldData : public WorldData
{
private:
	struct InteriorState
	{
		InteriorWorldData worldData;
		Int2 returnVoxel; // Where the player returns to outside.

		InteriorState(InteriorWorldData &&worldData, const Int2 &returnVoxel);
	};

	ExteriorLevelData levelData;
	std::unique_ptr<InteriorState> interior; // Non-null when the player is in an interior.
	bool isCity; // True if city, false if wilderness.

	ExteriorWorldData(ExteriorLevelData &&levelData, bool isCity);

	// Generates the .INF name for a city given a climate and current weather.
	static std::string generateCityInfName(ClimateType climateType, WeatherType weatherType);

	// Generates the .INF name for the wilderness given a climate and current weather.
	static std::string generateWildernessInfName(ClimateType climateType, WeatherType weatherType);
public:
	ExteriorWorldData(ExteriorWorldData&&) = default;
	virtual ~ExteriorWorldData();

	// Loads a premade exterior city (only used by center province).
	static ExteriorWorldData loadPremadeCity(const MIFFile &mif, ClimateType climateType,
		WeatherType weatherType, const ExeData &exeData);

	// Loads an exterior city skeleton and its random .MIF chunks.
	static ExteriorWorldData loadCity(int localCityID, int provinceID, const MIFFile &mif, int cityDim,
		const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
		WeatherType weatherType, const MiscAssets &miscAssets);

	// Loads some wilderness blocks.
	static ExteriorWorldData loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL,
		ClimateType climateType, WeatherType weatherType, const ExeData &exeData);

	// Returns the current active interior (if any).
	InteriorWorldData *getInterior() const;

	// This should be independent of any active interior (i.e., it always returns either city
	// or wilderness).
	virtual WorldType getWorldType() const override;

	virtual LevelData &getActiveLevel() override;
	virtual const LevelData &getActiveLevel() const override;

	// Sets the exterior world's active interior, and also saves the player's exterior voxel
	// position. Causes an error if there's already an interior.
	void enterInterior(InteriorWorldData &&interior, const Int2 &returnVoxel);

	// Leaves the current interior, sets the exterior active, and returns the saved voxel coordinate
	// for where the player is put in the exterior. Causes an error if no interior is active.
	Int2 leaveInterior();
};

#endif
