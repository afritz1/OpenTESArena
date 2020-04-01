#ifndef LOCATION_DEFINITION_H
#define LOCATION_DEFINITION_H

#include <string>

#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"

class LocationDefinition
{
public:
	enum class Type
	{
		City, // City/town/village.
		Dungeon, // Named quest dungeon. Wilderness 'random' den is not a location def.
		MainQuestDungeon // Start, map, or staff dungeon
	};

	struct CityDefinition
	{
		enum class Type { City, Town, Village };

		Type type;
		bool coastal;
		bool premade; // @todo: should be a nullable data struct instead, telling what kind of premade thing.

		void init(CityDefinition::Type type, bool coastal, bool premade);
	};

	struct DungeonDefinition
	{
		// Nothing
		void init();
	};

	struct MainQuestDungeonDefinition
	{
		enum class Type { Start, Map, Staff };

		// @todo: main quest code should look through world map definition to find start dungeon def
		// instead of assuming/hardcoding anything about its .MIF file. At that point, make a
		// StartDungeonDefinition to put the filename/etc. into.

		Type type;

		void init(MainQuestDungeonDefinition::Type type);
	};
private:
	std::string name;
	int x, y;
	bool visibleByDefault;

	// Determines union access.
	LocationDefinition::Type type;

	union
	{
		CityDefinition city;
		DungeonDefinition dungeon;
		MainQuestDungeonDefinition mainQuest;
	};

	void init(LocationDefinition::Type type, const CityDataFile::ProvinceData::LocationData &locationData);
public:
	// Initialize from original game data.
	void initCity(CityDefinition::Type type, const CityDataFile::ProvinceData::LocationData &locationData,
		bool coastal, bool premade);
	void initDungeon(const CityDataFile::ProvinceData::LocationData &locationData);
	void initMainQuestDungeon(MainQuestDungeonDefinition::Type type,
		const CityDataFile::ProvinceData::LocationData &locationData);
	// @todo: eventually have init(const char *filename) for custom locations.

	// Gets the display name of the location.
	const std::string &getName() const;

	// Pixel coordinates of the location.
	int getScreenX() const;
	int getScreenY() const;

	// Whether the location needs to be discovered for it to be visible on the map.
	bool isVisibleByDefault() const;

	// Gets the location type (city, dungeon, etc.). Determines union access.
	LocationDefinition::Type getType() const;

	// Type-specific location fields, accessible by the location type.
	const CityDefinition &getCityDefinition() const;
	const DungeonDefinition &getDungeonDefinition() const;
	const MainQuestDungeonDefinition &getMainQuestDungeonDefinition() const;
};

#endif
