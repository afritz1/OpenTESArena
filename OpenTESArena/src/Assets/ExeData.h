#ifndef EXE_DATA_H
#define EXE_DATA_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// This class stores data from the Arena executable. In other words, it represents a
// kind of "view" into the executable's data.

// When expanding this to work with both A.EXE and ACD.EXE, maybe use a union for
// members that differ between the two executables, with an _a/_acd suffix.

class KeyValueMap;

class ExeData
{
public:
	struct Calendar
	{
		std::array<std::string, 12> monthNames;
		std::array<std::string, 7> timesOfDay;
		std::array<std::string, 7> weekdayNames;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct CharacterClasses
	{
		// The allowed shields and weapons store 16-bit offsets from the start of the
		// .data segment into 0xFF-terminated ID arrays.
		std::array<uint8_t, 18> allowedArmors;
		std::array<uint16_t, 18> allowedShields;
		std::array<std::vector<uint8_t>, 5> allowedShieldsLists;

		// Added for convenience (evaluated from allowed shields and data segment offset);
		// points into allowedShieldsLists. -1 if "null".
		std::array<int, 18> allowedShieldsIndices;

		std::array<uint16_t, 18> allowedWeapons;
		std::array<std::vector<uint8_t>, 7> allowedWeaponsLists;

		// Added for convenience (evaluated from allowed weapons and data segment offset);
		// points into allowedWeaponsLists. -1 if "null".
		std::array<int, 18> allowedWeaponsIndices;

		// Character class names (ordered by: mages, thieves, warriors).
		std::array<std::string, 18> classNames;

		std::array<uint8_t, 18> classNumbersToIDs;
		std::array<uint8_t, 18> healthDice;
		std::array<uint16_t, 18> initialExperienceCaps;
		std::array<uint8_t, 18> lockpickingDivisors;
		std::array<std::string, 18> preferredAttributes;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct CharacterCreation
	{
		// Strings displayed during character creation.
		std::string chooseClassCreation;
		std::string chooseClassCreationGenerate;
		std::string chooseClassCreationSelect;
		std::string classQuestionsIntro;
		std::string suggestedClass;
		std::string chooseClassList;
		std::string chooseName;
		std::string chooseGender;
		std::string chooseGenderMale;
		std::string chooseGenderFemale;
		std::string chooseRace;
		std::string confirmRace;
		std::string confirmedRace1;
		std::string confirmedRace2;
		std::string confirmedRace3;
		std::string confirmedRace4;
		std::string distributeClassPoints;
		std::string chooseAttributes;
		std::string chooseAttributesSave;
		std::string chooseAttributesReroll;
		std::string chooseAppearance;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct CityGeneration
	{
		// IDs of locations on the coast (for water templates, etc.).
		std::array<uint8_t, 58> coastalCityList;

		// town%d.mif, ..., cityw%d.mif.
		std::array<std::string, 6> templateFilenames;

		// X and Y offsets from the city origin.
		std::array<std::pair<uint8_t, uint8_t>, 22> startingPositions;

		// Reserved blocks in the city plan.
		std::array<std::vector<uint8_t>, 8> reservedBlockLists;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Entities
	{
		// Creature names ("Rat", "Goblin", etc.). Each creature type could simply use
		// its index in this array as its identifier, much like with provinces.
		std::array<std::string, 23> names;

		// Attribute arrays for male/female races, guards, and creatures. 255 is displayed
		// as 100.
		std::array<std::array<uint8_t, 8>, 8> maleMainRaceAttributes, femaleMainRaceAttributes;
		std::array<std::array<uint8_t, 8>, 9> guardAttributes;
		std::array<std::array<uint8_t, 8>, 24> creatureAttributes;

		// Creature animations (i.e., their .CFA filenames). These are ordered the same
		// as creature names, and there is an extra entry at the end for the final boss.
		// Replace '@' with a number from 1 to 6 indicating which .CFA file to fetch for
		// angle-relative animations.
		std::array<std::string, 24> animationFilenames;

		// Random male citizen .CFA filenames. Replace '@' with a number from 1 to 5.
		// - Order: Winter, Desert, Temperate.
		std::array<std::string, 3> maleCitizenAnimationFilenames;

		// Random female citizen .CFA filenames. Replace '@' with a number from 1 to 5.
		// - Order: Temperate, Desert, Winter.
		std::array<std::string, 3> femaleCitizenAnimationFilenames;

		// Filename chunks for sprites with variable weapons, etc., to be combined with
		// the .CFA filenames containing three X's (walk, attack, bow).
		// - Order: Plate, Chain, Leather, Unarmored, Mage, Monk, Barbarian.
		std::array<std::string, 7> cfaFilenameChunks;

		// Filename templates to be used with the .CFA filename chunks. Replace "0@" with
		// either 0 or 1 for the gender, and 1 to 5 depending on the kind of animation (i.e.,
		// attacks are only forward, so they don't have 2 through 5). Replace "XXX" with one
		// of the .CFA filename chunks. Not every character has every combination (i.e.,
		// barbarians don't have a bow shoot animation, and there is no female plate).
		// - Order: Walk, Attack, Bow.
		std::array<std::string, 3> cfaFilenameTemplates;

		// A few premade human .CFA animations with some weapons. Some of these can't be 
		// created from pairing a.CFA template with a .CFA chunk because there aren't the
		// right selection of templates available (I think... because it's kind of messy).
		// - Order: Mage + sword, Mage + staff, Mage spell, Monk kick.
		std::array<std::string, 4> cfaHumansWithWeaponAnimations;

		// Human .CFA weapons by themselves, presumably for combining with empty-handed 
		// animations.
		// - Order: Sword, Axe, Mace, "P" sword, "P" axe, "P" mace, "B" sword, "B" axe,
		//   "B" mace, Shield, "P" shield, "B" shield.
		std::array<std::string, 12> cfaWeaponAnimations;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Equipment
	{
		std::array<std::string, 7> bodyPartNames; // Chest, ..., foot.
		std::array<std::string, 7> armorNames; // Cuirass, ..., boots.
		std::array<std::string, 4> shieldNames; // Buckler, ..., tower shield.
		std::array<std::string, 18> weaponNames; // Staff, ..., long bow.
		std::array<std::string, 8> metalNames;
		std::array<std::string, 11> weaponAnimationFilenames; // staff.cif, ..., spell.img.

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Locations
	{
		// Province names, ordered by appearance on the world map reading from left to
		// right, with the center province last.
		std::array<std::string, 9> provinceNames;

		// The character creation province names have slight changes and leave out the
		// center province.
		std::array<std::string, 8> charCreationProvinceNames;

		// Province .IMG backgrounds (ordered the same as province names).
		std::array<std::string, 9> provinceImgFilenames;

		// City-state, town, village (stored twice), dungeon.
		std::array<std::string, 5> locationTypes;

		// Display name of the initial dungeon.
		std::string startDungeonName;

		// Indices for each province that has a staff piece, ordered as they are found in-game.
		std::array<uint8_t, 8> staffProvinces;

		// Climates for each global quarter. Used with weather calculation.
		std::array<uint8_t, 36> climates;

		// Weathers for each global quarter climate + season + variant tuple.
		std::array<uint8_t, 140> weatherTable;

		// Twelve month-wise travel speed modifiers for each climate.
		std::array<std::array<uint8_t, 12>, 7> climateSpeedTables;

		// Eight weather-wise travel speed modifiers for each climate. 0 represents 100.
		std::array<std::array<uint8_t, 8>, 7> weatherSpeedTables;

		// Wilderness .RMD index lists for each type of block (normal, village, dungeon,
		// inn, and temple). Each list starts with the number of elements.
		std::vector<uint8_t> wildernessNormalBlocks, wildernessVillageBlocks,
			wildernessDungeonBlocks, wildernessInnBlocks, wildernessTempleBlocks;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Logbook
	{
		std::string logbookIsEmpty;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Meta
	{
		// The data segment offset is used to find certain data, like allowed shields and
		// weapons lists (not necessary for this class, though).
		uint32_t dataSegmentOffset;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Races
	{
		// Race names (ordered the same as provinces).
		std::array<std::string, 8> singularNames;
		std::array<std::string, 8> pluralNames;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Status
	{
		// Status pop-up text (with %s/%d tokens).
		std::string popUp;

		// Weekday/day/month/year text (with %s tokens), used in pop-up.
		std::string date;

		std::string fortify; // With %s token.
		std::string disease; // With %s token.
		std::string effect; // With %s token.
		std::array<std::string, 23> effectsList; // Healthy, diseased, etc..

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct Travel
	{
		// Location format texts when clicking on a location on a province map.
		// Each one is phrased a little differently:
		// - <dungeon> in <province> Province
		// - The <center province city> in the <center province>
		// - The <city type> of <city name> in <province> Province
		std::array<std::string, 3> locationFormatTexts;

		// Description for how many days traveling will take based on the current weather,
		// split into two strings for some reason.
		std::array<std::string, 2> dayPrediction;

		// Description for how many kilometers the journey will take.
		std::string distancePrediction;

		// Description for when the player should arrive at their destination.
		std::string arrivalDatePrediction;

		// Pop-up when the player tries to travel to their current location.
		std::string alreadyAtDestination;

		// Pop-up when no destination is selected.
		std::string noDestination;

		// Location/date/days text when the player arrives at their destination.
		std::string arrivalPopUpLocation, arrivalPopUpDate, arrivalPopUpDays;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};

	struct WallHeightTables
	{
		// Values for interior and exterior wall heights. In wilderness cells, the values in
		// box1 and box2 are multiplied by 192/256. In interiors, they are also scaled by the
		// ceiling2 value (second value in *CEILING lines; default=128?).
		std::array<uint16_t, 8> box1a, box1b, box1c;
		std::array<uint16_t, 16> box2a, box2b;
		// Ignore "source" array, a copy of previous 56 words.
		std::array<uint16_t, 8> box3a, box3b;
		std::array<uint16_t, 16> box4;

		void init(const char *data, const KeyValueMap &keyValueMap);
	};
private:
	static const std::string FLOPPY_VERSION_EXE_FILENAME;
	static const std::string FLOPPY_VERSION_MAP_FILENAME;
	static const std::string CD_VERSION_EXE_FILENAME;
	static const std::string CD_VERSION_MAP_FILENAME;
	static const char PAIR_SEPARATOR;

	// Gets the offset value from the given key.
	static int get(const std::string &key, const KeyValueMap &keyValueMap);

	// Gets the offset + length value from the given key.
	static std::pair<int, int> getPair(const std::string &key, const KeyValueMap &keyValueMap);

	static int8_t readInt8(const char *data);
	static uint8_t readUint8(const char *data);
	static int16_t readInt16(const char *data);
	static uint16_t readUint16(const char *data);
	static std::string readString(const char *data); // Null-terminated string.
	static std::string readString(const char *data, int length); // Fixed-size string.

	// Convenience method for reading a fixed-size string given the .EXE text pointer
	// and an offset + length pair.
	static std::string readFixedString(const char *data, const std::pair<int, int> &pair);

	bool floppyVersion;
public:
	Calendar calendar;
	CharacterClasses charClasses;
	CharacterCreation charCreation;
	CityGeneration cityGen;
	Entities entities;
	Equipment equipment;
	Locations locations;
	Logbook logbook;
	Meta meta;
	Races races;
	Status status;
	Travel travel;
	WallHeightTables wallHeightTables;

	bool isFloppyVersion() const;

	// The floppy version boolean determines which strings file to use, and potentially
	// how to interpret various data structures in the executable.
	void init(bool floppyVersion);
};

#endif
