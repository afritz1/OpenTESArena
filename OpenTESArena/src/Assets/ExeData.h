#ifndef EXE_DATA_H
#define EXE_DATA_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "ExeTypes.h"

#include "components/utilities/KeyValueFile.h"

// This class stores data from the Arena executable. In other words, it represents a
// kind of "view" into the executable's data.

// When expanding this to work with both A.EXE and ACD.EXE, maybe use a union for
// members that differ between the two executables, with an _a/_acd suffix.

class KeyValueFile;

class ExeData
{
public:
	struct Calendar
	{
		std::array<std::string, 12> monthNames;
		std::array<std::string, 7> timesOfDay;
		std::array<std::string, 7> weekdayNames;
		std::array<std::string, 15> holidayNames;
		std::array<uint16_t, 15> holidayDates;

		void init(const char *data, const KeyValueFile &keyValueFile);
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

		void init(const char *data, const KeyValueFile &keyValueFile);
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

		void init(const char *data, const KeyValueFile &keyValueFile);
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

		// Prefixes and suffixes for taverns/temples/equipment stores.
		std::array<std::string, 23> tavernPrefixes;
		std::array<std::string, 23> tavernMarineSuffixes;
		std::array<std::string, 23> tavernSuffixes;
		std::array<std::string, 3> templePrefixes;
		std::array<std::string, 5> temple1Suffixes;
		std::array<std::string, 9> temple2Suffixes;
		std::array<std::string, 10> temple3Suffixes;
		std::array<std::string, 20> equipmentPrefixes;
		std::array<std::string, 10> equipmentSuffixes;

		// The displayed name when a mage's guild *MENU voxel is right-clicked.
		std::string magesGuildMenuName;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct Entities
	{
		// Creature races are 1-based.
		std::array<std::string, 23> creatureNames; // Rat, Goblin, ...
		std::array<uint8_t, 24> creatureLevels; // Zero-based, as with the player.
		std::array<std::pair<uint16_t, uint16_t>, 24> creatureHitPoints; // Min/max format.
		std::array<uint32_t, 24> creatureBaseExps; // Exp = baseExp + (maxHP * expMultiplier).
		std::array<uint8_t, 24> creatureExpMultipliers;
		std::array<uint8_t, 24> creatureSounds; // Indices into creature .VOC filenames.
		std::array<std::string, 26> creatureSoundNames; // Used by creature sound indices.
		std::array<std::pair<uint8_t, uint8_t>, 24> creatureDamages; // Min/max format.
		std::array<uint16_t, 24> creatureMagicEffects; // Goes into NPC's ActiveEffects.
		std::array<uint16_t, 24> creatureScales; // In 1/256th's, 0 == 100%.
		std::array<int8_t, 24> creatureYOffsets;
		std::array<uint8_t, 24> creatureHasNoCorpse;
		std::array<uint8_t, 24> creatureBlood; // Indices into effects animation list.
		std::array<int8_t, 24> creatureDiseaseChances; // Negative values have special meaning.
		std::array<std::array<uint8_t, 8>, 24> creatureAttributes; // 255 == 100.

		// Display name of the final boss when selecting them.
		std::string finalBossName;

		// Creature animations (i.e., their .CFA filenames). These are ordered the same
		// as creature names, and there is an extra entry at the end for the final boss.
		// Replace '@' with a number from 1 to 6 indicating which .CFA file to fetch for
		// angle-relative animations.
		std::array<std::string, 24> creatureAnimationFilenames;

		// Attribute arrays for male/female races and guards. 255 is displayed as 100.
		// The race arrays alternate male/female/male/female.
		std::array<std::array<uint8_t, 8>, 16> raceAttributes;
		std::array<std::array<uint8_t, 8>, 9> guardAttributes;

		// Random male citizen .CFA filenames. Replace '@' with a number from 1 to 5.
		// - Order: Winter, Desert, Temperate.
		std::array<std::string, 3> maleCitizenAnimationFilenames;

		// Random female citizen .CFA filenames. Replace '@' with a number from 1 to 5.
		// - Order: Temperate, Desert, Winter.
		std::array<std::string, 3> femaleCitizenAnimationFilenames;

		// Filename chunks for sprites with variable weapons, etc., to be combined with
		// the .CFA filenames containing three X's (walk, attack, bow).
		// - Order: Plate, Chain, Leather, Unarmored, Mage, Monk, Barbarian.
		std::array<std::string, 7> humanFilenameTypes;

		// Filename templates to be used with the .CFA filename chunks. Replace "0@" with
		// either 0 or 1 for the gender, and 1 to 5 depending on the kind of animation (i.e.,
		// attacks are only forward, so they don't have 2 through 5). Replace "XXX" with one
		// of the .CFA filename chunks. Not every character has every combination (i.e.,
		// barbarians don't have a bow shoot animation, and there is no female plate).
		// - Order: Walk, Attack, Bow.
		std::array<std::string, 3> humanFilenameTemplates;

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

		// .CFA filenames for spell explosions, blood, etc..
		std::array<std::string, 27> effectAnimations;

		// Townsfolk clothes and skin color transformations. See NPCs wiki page for the algorithms.
		std::array<uint8_t, 16> citizenColorBase;
		std::array<uint8_t, 10> citizenSkinColors;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	// See Items wiki page for more information.
	struct Equipment
	{
		// Three chances for what enchantment is selected; a random number 1..10 is tested
		// against it. 0: special material, 1: enchantment, 2: material + enchantment.
		std::array<uint8_t, 3> enchantmentChances;

		// Material values.
		std::array<std::string, 8> materialNames;
		std::array<int8_t, 8> materialBonuses;
		std::array<uint8_t, 8> materialChances;
		std::array<uint16_t, 8> materialPriceMultipliers; // In quarters.

		// Plate armor values (including shields).
		std::array<std::string, 11> armorNames; // Cuirass, ..., tower shield.
		std::array<std::string, 11> plateArmorNames; // Plate cuirass, ..., tower shield.
		std::array<uint8_t, 11> plateArmorQualities;
		std::array<uint8_t, 11> plateArmorBasePrices;
		std::array<uint16_t, 11> plateArmorWeights; // In kg/256.

		// Chain armor values (including shields).
		std::array<std::string, 11> chainArmorNames; // Chain cuirass, ..., tower shield.
		std::array<uint8_t, 11> chainArmorQualities;
		std::array<uint8_t, 11> chainArmorBasePrices;
		std::array<uint16_t, 11> chainArmorWeights; // In kg/256.

		// Leather armor values (including shields).
		std::array<std::string, 11> leatherArmorNames; // Leather cuirass, ..., tower shield.
		std::array<uint8_t, 11> leatherArmorQualities;
		std::array<uint8_t, 11> leatherArmorBasePrices;
		std::array<uint16_t, 11> leatherArmorWeights; // In kg/256.

		// Shield armor classes, used with armor class bonuses for shields.
		std::array<uint8_t, 4> shieldArmorClasses;

		// Armor enchantment values.
		std::array<std::string, 14> armorEnchantmentNames;
		std::array<uint8_t, 14> armorEnchantmentQualities;
		std::array<uint8_t, 14> armorEnchantmentSpells; // 255 means attribute bonus instead of spell.
		std::array<uint8_t, 14> armorEnchantmentBonusPrices;

		// Weapon values.
		std::array<std::string, 18> weaponNames; // Staff, ..., long bow.
		std::array<uint8_t, 18> weaponQualities;
		std::array<uint8_t, 18> weaponBasePrices;
		std::array<uint16_t, 18> weaponWeights; // In kg/256.
		std::array<std::pair<uint8_t, uint8_t>, 18> weaponDamages; // Min/max pairs.
		std::array<uint8_t, 18> weaponHandednesses;

		// Weapon enchantment values.
		std::array<std::string, 14> weaponEnchantmentNames;
		std::array<uint8_t, 14> weaponEnchantmentQualities;
		std::array<uint8_t, 14> weaponEnchantmentSpells; // See armor enchantment note.
		std::array<uint8_t, 14> weaponEnchantmentBonusPrices;

		// Three categories of trinkets: spellcasting items, attribute enhancement items,
		// and armor class items. Spellcasting items are split into offense/defense/misc.
		std::array<std::string, 4> spellcastingItemNames;
		std::array<uint8_t, 4> spellcastingItemCumulativeChances;
		std::array<uint8_t, 4> spellcastingItemBasePrices;
		std::array<std::pair<uint8_t, uint8_t>, 4> spellcastingItemChargeRanges; // Min/max + 1...
		std::array<std::string, 15> spellcastingItemAttackSpellNames;
		std::array<uint8_t, 15> spellcastingItemAttackSpellQualities;
		std::array<uint8_t, 15> spellcastingItemAttackSpellSpells;
		std::array<uint8_t, 15> spellcastingItemAttackSpellPricesPerCharge;
		std::array<std::string, 9> spellcastingItemDefensiveSpellNames;
		std::array<uint8_t, 9> spellcastingItemDefensiveSpellQualities;
		std::array<uint8_t, 9> spellcastingItemDefensiveSpellSpells;
		std::array<uint8_t, 9> spellcastingItemDefensiveSpellPricesPerCharge;
		std::array<std::string, 8> spellcastingItemMiscSpellNames;
		std::array<uint8_t, 9> spellcastingItemMiscSpellQualities;
		std::array<uint8_t, 9> spellcastingItemMiscSpellSpells;
		std::array<uint8_t, 9> spellcastingItemMiscSpellPricesPerCharge;
		std::array<std::string, 4> enhancementItemNames;
		std::array<uint8_t, 4> enhancementItemCumulativeChances;
		std::array<uint8_t, 4> enhancementItemBasePrices;

		// @todo: artifacts.

		std::array<std::string, 11> bodyPartNames; // Chest, ..., general.
		std::array<std::string, 11> weaponAnimationFilenames; // staff.cif, ..., spell.img.

		void init(const char *data, const KeyValueFile &keyValueFile);
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

		// Palace, bs, noble, ..., vilpal, tower. Used with *MENU voxel .MIF filenames.
		std::array<std::string, 11> menuMifPrefixes;

		// .MIF name of the center province's city.
		std::string centerProvinceCityMifName;

		// Display name of the initial dungeon.
		std::string startDungeonName;

		// .MIF names of the initial and final main quest dungeons.
		std::string startDungeonMifName, finalDungeonMifName;

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

		// Ruler titles for cities.
		std::array<std::string, 14> rulerTitles;

		// Filenames for mountains drawn on the horizon.
		std::array<std::string, 3> distantMountainFilenames;

		// Filenames for animated mountains (i.e., volcanoes).
		std::array<std::string, 3> animDistantMountainFilenames;

		// Base filename for distant clouds.
		std::string cloudFilename;

		// Filename for the sun.
		std::string sunFilename;

		// Filenames for the moons.
		std::array<std::string, 2> moonFilenames;

		// Base filename for distant stars.
		std::string starFilename;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct Logbook
	{
		std::string isEmpty;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct Meta
	{
		// The data segment offset is used to find certain data, like allowed shields and
		// weapons lists (not necessary for this class, though).
		uint32_t dataSegmentOffset;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct Quests
	{
		std::array<std::string, 8> mainQuestItemNames;

		// Staff piece count in inventory.
		std::string staffPieces;

		std::array<std::string, 12> keyNames;
		std::string keyPickedUp;
		std::string doorUnlockedWithKey;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct Races
	{
		// Race names (ordered the same as provinces).
		std::array<std::string, 8> singularNames;
		std::array<std::string, 8> pluralNames;

		void init(const char *data, const KeyValueFile &keyValueFile);
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

		void init(const char *data, const KeyValueFile &keyValueFile);
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

		// Unique string for the center province's city in the arrival pop-up.
		std::string arrivalCenterProvinceLocation;

		// Title text in location search pop-up.
		std::string searchTitleText;

		// Filenames for each staff dungeon splash image.
		std::array<std::string, 8> staffDungeonSplashes;

		// Province indices into the staff dungeon splash filenames.
		std::array<uint8_t, 8> staffDungeonSplashIndices;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct UI
	{
		ExeTypes::List chooseClassList;
		ExeTypes::List buyingWeapons; // POPUP3.
		ExeTypes::List buyingArmor; // POPUP4.
		ExeTypes::List spellmaker; // POPUP.
		ExeTypes::List popUp5;
		ExeTypes::List loadSave;
		ExeTypes::List charClassSelection; // POPUP2.
		ExeTypes::List buyingMagicItems; // POPUP7.
		ExeTypes::List travelCitySelection; // POPUP8.
		ExeTypes::List dialogue; // POPUP11.
		ExeTypes::List roomSelectionAndCures; // NEWPOP.
		ExeTypes::List generalLootAndSelling; // NEWPOP.

		// In X, Y format.
		std::array<uint16_t, 16> followerPortraitPositions;

		// Armor class numbers on character sheet. In X, Y format.
		std::array<uint16_t, 14> maleArmorClassPositions, femaleArmorClassPositions;

		// Thirty palette indices to translate some races' helmet skin colors with.
		// "race#" points to the race in "province #". Race 7 is a special case; they
		// use "xLIZn.IMG" instead, where 'x' is either 'M' or 'F', and 'n' is the
		// armor type (plate=0, etc.).
		std::array<uint8_t, 30> helmetPaletteIndices, race1HelmetPaletteValues,
			race3HelmetPaletteValues, race4HelmetPaletteValues;

		// Displayed when pressing F2.
		std::string currentWorldPosition;

		// Displayed when clicking an entity.
		std::string inspectedEntityName;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct WallHeightTables
	{
		// Values for interior and exterior wall heights and texture mapping.

		// If ceiling2 is given, the box value is scaled with (x * ceiling2) / 256. Otherwise,
		// if it is wilderness, the scale value is 192.

		// In the voxel data, the most significant byte of raised platforms contains the thickness
		// and height indices, organized as 0x0tttthhh.

		// Box1: raised platform heights
		// Box2: raised platform thicknesses
		// Box3 and Box4: texture coordinates? Possible bug that Box4 is used instead of Box3c.
		// 'a': interiors/dungeons
		// 'b': cities
		// 'c': wilderness

		std::array<uint16_t, 8> box1a, box1b, box1c;
		std::array<uint16_t, 16> box2a, box2b;
		// Ignore "source" array, a copy of previous 56 words.
		std::array<uint16_t, 8> box3a, box3b;
		std::array<uint16_t, 16> box4;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};

	struct Wilderness
	{
		// .RMD index lists for each type of block (normal, village, dungeon, tavern, and temple).
		// Each list starts with the number of elements.
		std::vector<uint8_t> normalBlocks, villageBlocks, dungeonBlocks,
			tavernBlocks, templeBlocks;

		void init(const char *data, const KeyValueFile &keyValueFile);
	};
private:
	static const std::string CD_VERSION_MAP_FILENAME;
	static const std::string FLOPPY_VERSION_MAP_FILENAME;
	static constexpr char PAIR_SEPARATOR = ',';

	// Gets the offset value from the given section and key.
	static int get(const KeyValueFile::Section &section, const std::string &key);

	// Gets the offset + length value from the given section and key.
	static std::pair<int, int> getPair(const KeyValueFile::Section &section, const std::string &key);

	static int8_t readInt8(const char *data);
	static uint8_t readUint8(const char *data);
	static int16_t readInt16(const char *data);
	static uint16_t readUint16(const char *data);
	static std::string readString(const char *data); // Null-terminated string.

	// Convenience method for reading a fixed-size string given the .EXE text pointer
	// and an offset + length pair.
	static std::string readFixedString(const char *data, const std::pair<int, int> &pair);

	bool floppyVersion;
public:
	static const std::string CD_VERSION_EXE_FILENAME;
	static const std::string FLOPPY_VERSION_EXE_FILENAME;

	Calendar calendar;
	CharacterClasses charClasses;
	CharacterCreation charCreation;
	CityGeneration cityGen;
	Entities entities;
	Equipment equipment;
	Locations locations;
	Logbook logbook;
	Meta meta;
	Quests quests;
	Races races;
	Status status;
	Travel travel;
	UI ui;
	WallHeightTables wallHeightTables;
	Wilderness wild;

	bool isFloppyVersion() const;

	// The floppy version boolean determines which strings file to use, and potentially
	// how to interpret various data structures in the executable.
	bool init(bool floppyVersion);
};

#endif
