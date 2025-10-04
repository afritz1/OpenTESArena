#ifndef EXE_DATA_H
#define EXE_DATA_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "ExeTypes.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span.h"

class KeyValueFile;
class KeyValueFileSection;

enum class MapType;

struct ExeDataCalendar
{
	std::string monthNames[12];
	std::string timesOfDay[7];
	std::string weekdayNames[7];
	std::string holidayNames[15];
	uint16_t holidayDates[15];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataCharacterClasses
{
	// The allowed shields and weapons store 16-bit offsets from the start of the
	// .data segment into 0xFF-terminated ID arrays.
	uint8_t allowedArmors[18];
	uint16_t allowedShields[18];
	std::vector<uint8_t> allowedShieldsLists[5];

	// Added for convenience (evaluated from allowed shields and data segment offset);
	// points into allowedShieldsLists. -1 if "null".
	int allowedShieldsIndices[18];

	uint16_t allowedWeapons[18];
	std::vector<uint8_t> allowedWeaponsLists[7];

	// Added for convenience (evaluated from allowed weapons and data segment offset);
	// points into allowedWeaponsLists. -1 if "null".
	int allowedWeaponsIndices[18];

	// Character class names (ordered by: mages, thieves, warriors).
	std::string classNames[18];

	uint8_t classNumbersToIDs[18];
	uint8_t healthDice[18];
	uint16_t initialExperienceCaps[18];
	uint8_t lockpickingDivisors[18];
	std::string preferredAttributes[18];
	uint8_t magicClassIntelligenceMultipliers[7];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataCharacterCreation
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
	std::string chooseAttributesBonusPointsRemaining;
	std::string chooseAppearance;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataCityGeneration
{
	// IDs of locations on the coast (for water templates, etc.).
	uint8_t coastalCityList[58];

	// town%d.mif, ..., cityw%d.mif.
	std::string templateFilenames[6];

	// X and Y offsets from the city origin.
	std::pair<uint8_t, uint8_t> startingPositions[22];

	// Reserved blocks in the city plan.
	std::vector<uint8_t> reservedBlockLists[8];

	// Prefixes and suffixes for taverns/temples/equipment stores.
	std::string tavernPrefixes[23];
	std::string tavernMarineSuffixes[23];
	std::string tavernSuffixes[23];
	std::string templePrefixes[3];
	std::string temple1Suffixes[5];
	std::string temple2Suffixes[9];
	std::string temple3Suffixes[10];
	std::string equipmentPrefixes[20];
	std::string equipmentSuffixes[10];

	// The displayed name when a mage's guild *MENU voxel is right-clicked.
	std::string magesGuildMenuName;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataEntities
{
	std::string attributeNames[8]; // Strength, ... Luck.

	// Creature races are 1-based.
	std::string creatureNames[23]; // Rat, Goblin, ...
	uint8_t creatureLevels[24]; // Zero-based, as with the player.
	std::pair<uint16_t, uint16_t> creatureHitPoints[24]; // Min/max format.
	uint32_t creatureBaseExps[24]; // Exp = baseExp + (maxHP * expMultiplier).
	uint8_t creatureExpMultipliers[24];
	uint8_t creatureSounds[24]; // Indices into creature .VOC filenames.
	std::string creatureSoundNames[26]; // Used by creature sound indices.
	std::pair<uint8_t, uint8_t> creatureDamages[24]; // Min/max format.
	uint16_t creatureMagicEffects[24]; // Goes into NPC's ActiveEffects.
	uint16_t creatureScales[24]; // In 1/256th's, 0 == 100%.
	int8_t creatureYOffsets[24];
	uint8_t creatureHasNoCorpse[24];
	uint8_t creatureBlood[24]; // Indices into effects animation list.
	int8_t creatureDiseaseChances[24]; // Negative values have special meaning.
	uint8_t creatureAttributes[24][8]; // 255 == 100.
	uint32_t creatureLootChances[24]; // Accessed by 1-based races

	// Creature animations (i.e., their .CFA filenames). These are ordered the same
	// as creature names, and there is an extra entry at the end for the final boss.
	// Replace '@' with a number from 1 to 6 indicating which .CFA file to fetch for
	// angle-relative animations.
	std::string creatureAnimationFilenames[24];

	// Display name of the final boss when selecting them.
	std::string finalBossName;

	// Attribute arrays for male/female races and guards. 255 is displayed as 100.
	// The race arrays alternate male/female/male/female.
	uint8_t raceAttributes[16][8];
	uint8_t guardAttributes[9][8];

	// Random male citizen .CFA filenames. Replace '@' with a number from 1 to 5.
	// - Order: Winter, Desert, Temperate.
	std::string maleCitizenAnimationFilenames[3];

	// Random female citizen .CFA filenames. Replace '@' with a number from 1 to 5.
	// - Order: Temperate, Desert, Winter.
	std::string femaleCitizenAnimationFilenames[3];

	// Filename chunks for sprites with variable weapons, etc., to be combined with
	// the .CFA filenames containing three X's (walk, attack, bow).
	// - Order: Plate, Chain, Leather, Unarmored, Mage, Monk, Barbarian.
	std::string humanFilenameTypes[7];

	// Filename templates to be used with the .CFA filename chunks. Replace "0@" with
	// either 0 or 1 for the gender, and 1 to 5 depending on the kind of animation (i.e.,
	// attacks are only forward, so they don't have 2 through 5). Replace "XXX" with one
	// of the .CFA filename chunks. Not every character has every combination (i.e.,
	// barbarians don't have a bow shoot animation, and there is no female plate).
	// - Order: Walk, Attack, Bow.
	std::string humanFilenameTemplates[3];

	// A few premade human .CFA animations with some weapons. Some of these can't be 
	// created from pairing a.CFA template with a .CFA chunk because there aren't the
	// right selection of templates available (I think... because it's kind of messy).
	// - Order: Mage + sword, Mage + staff, Mage spell, Monk kick.
	std::string cfaHumansWithWeaponAnimations[4];

	// Human .CFA weapons by themselves, presumably for combining with empty-handed 
	// animations.
	// - Order: Sword, Axe, Mace, "P" sword, "P" axe, "P" mace, "B" sword, "B" axe,
	//   "B" mace, Shield, "P" shield, "B" shield.
	std::string cfaWeaponAnimations[12];

	// .CFA filenames for spell explosions, blood, etc..
	std::string effectAnimations[27];

	// Townsfolk clothes and skin color transformations. See NPCs wiki page for the algorithms.
	uint8_t citizenColorBase[16];
	uint8_t citizenSkinColors[10];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

// See Items wiki page for more information.
struct ExeDataEquipment
{
	// Three chances for what enchantment is selected; a random number 1..10 is tested
	// against it. 0: special material, 1: enchantment, 2: material + enchantment.
	uint8_t enchantmentChances[3];

	// Material values.
	std::string materialNames[8];
	int8_t materialBonuses[8];
	uint8_t materialChances[8];
	uint16_t materialPriceMultipliers[8]; // In quarters.

	// Condition/degradation values.
	std::string itemConditionNames[8]; // New, used, ...

	// Plate armor values (including shields).
	std::string armorNames[11]; // Cuirass, ..., tower shield.
	std::string plateArmorNames[11]; // Plate cuirass, ..., tower shield.
	uint8_t plateArmorQualities[11];
	uint8_t plateArmorBasePrices[11];
	uint16_t plateArmorWeights[11]; // In kg/256.

	// Chain armor values (including shields).
	std::string chainArmorNames[11]; // Chain cuirass, ..., tower shield.
	uint8_t chainArmorBasePrices[11];
	uint8_t chainArmorQualities[11];
	uint16_t chainArmorWeights[11]; // In kg/256.

	// Leather armor values (including shields).
	std::string leatherArmorNames[11]; // Leather cuirass, ..., tower shield.
	uint8_t leatherArmorQualities[11];
	uint8_t leatherArmorBasePrices[11];
	uint16_t leatherArmorWeights[11]; // In kg/256.

	// Shield armor classes, used with armor class bonuses for shields.
	uint8_t shieldArmorClasses[4];

	// Armor enchantment values.
	std::string armorEnchantmentNames[14];
	uint8_t armorEnchantmentQualities[14];
	uint8_t armorEnchantmentSpells[14]; // 255 means attribute bonus instead of spell.
	uint16_t armorEnchantmentBonusPrices[14];

	// Weapon values.
	std::string weaponNames[18]; // Staff, ..., long bow.
	uint8_t weaponQualities[18];
	uint8_t weaponBasePrices[18];
	uint16_t weaponWeights[18]; // In kg/256.
	std::pair<uint8_t, uint8_t> weaponDamages[18]; // Min/max pairs.
	uint8_t weaponHandednesses[18];

	// Weapon enchantment values.
	std::string weaponEnchantmentNames[14];
	uint8_t weaponEnchantmentQualities[14];
	uint8_t weaponEnchantmentSpells[14]; // See armor enchantment note.
	uint16_t weaponEnchantmentBonusPrices[14];

	// Three categories of trinkets: spellcasting items, attribute enhancement items,
	// and armor class items. Spellcasting items are split into offense/defense/misc.
	std::string spellcastingItemNames[4];
	uint8_t spellcastingItemCumulativeChances[4];
	uint16_t spellcastingItemBasePrices[4];
	std::pair<uint8_t, uint8_t> spellcastingItemChargeRanges[4]; // Min/max + 1...
	std::string spellcastingItemAttackSpellNames[15];
	uint8_t spellcastingItemAttackSpellQualities[15];
	uint8_t spellcastingItemAttackSpellSpells[15];
	uint16_t spellcastingItemAttackSpellPricesPerCharge[15];
	std::string spellcastingItemDefensiveSpellNames[9];
	uint8_t spellcastingItemDefensiveSpellQualities[9];
	uint8_t spellcastingItemDefensiveSpellSpells[9];
	uint16_t spellcastingItemDefensiveSpellPricesPerCharge[9];
	std::string spellcastingItemMiscSpellNames[8];
	uint8_t spellcastingItemMiscSpellQualities[8];
	uint8_t spellcastingItemMiscSpellSpells[8];
	uint16_t spellcastingItemMiscSpellPricesPerCharge[8];
	std::string enhancementItemNames[4];
	uint8_t enhancementItemCumulativeChances[4];
	uint16_t enhancementItemBasePrices[4];

	// Consumables.
	std::string potionNames[15]; // "Potion of <effect>"...
	std::string unidentifiedPotionName;

	// @todo: artifacts.

	std::string bodyPartNames[11]; // Chest, ..., general.
	std::string weaponAnimationFilenames[11]; // staff.cif, ..., spell.img.

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataItems
{
	// In container inventory.
	std::string goldPiece;
	std::string bagOfGoldPieces;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataLight
{
	uint8_t windowTwilightColors[48]; // VGA (6-bit, 63 = 255) RGB triplets for window color transition.
	uint16_t waterTwilightLightLevels[16];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataLocations
{
	// Province names, ordered by appearance on the world map reading from left to
	// right, with the center province last.
	std::string provinceNames[9];

	// The character creation province names have slight changes and leave out the
	// center province.
	std::string charCreationProvinceNames[8];

	// Province .IMG backgrounds (ordered the same as province names).
	std::string provinceImgFilenames[9];

	// City-state, town, village (stored twice), dungeon.
	std::string locationTypes[5];

	// Palace, bs, noble, ..., vilpal, tower. Used with *MENU voxel .MIF filenames.
	std::string menuMifPrefixes[11];

	// .MIF name of the center province's city.
	std::string centerProvinceCityMifName;

	// Display name of the initial dungeon.
	std::string startDungeonName;

	// .MIF names of the initial and final main quest dungeons.
	std::string startDungeonMifName, finalDungeonMifName;

	// Indices for each province that has a staff piece, ordered as they are found in-game.
	uint8_t staffProvinces[8];

	// Climates for each global quarter. Used with weather calculation.
	uint8_t climates[36];

	// Weathers for each global quarter climate + season + variant tuple.
	uint8_t weatherTable[140];

	// Twelve month-wise travel speed modifiers for each climate.
	uint8_t climateSpeedTables[7][12];

	// Eight weather-wise travel speed modifiers for each climate. 0 represents 100.
	uint8_t weatherSpeedTables[7][8];

	// Ruler titles for cities.
	std::string rulerTitles[14];

	// Filenames for mountains drawn on the horizon.
	std::string distantMountainFilenames[3];

	// Filenames for animated mountains (i.e., volcanoes).
	std::string animDistantMountainFilenames[3];

	// Base filename for distant clouds.
	std::string cloudFilename;

	// Filename for the sun.
	std::string sunFilename;

	// Filenames for the moons.
	std::string moonFilenames[2];

	// Base filename for distant stars.
	std::string starFilename;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataLogbook
{
	std::string isEmpty;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataMath
{
	int16_t cosineTable[641];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataMeta
{
	// The data segment offset is used to find certain data, like allowed shields and
	// weapons lists (not necessary for this class, though).
	uint32_t dataSegmentOffset;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataQuests
{
	std::string mainQuestItemNames[8];

	// Staff piece count in inventory.
	std::string staffPieces;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataRaces
{
	// Race names (ordered the same as provinces).
	std::string singularNames[8];
	std::string pluralNames[8];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataRaisedPlatforms
{
	// Heights, sizes, and texture mapping for interior and exterior raised platforms.

	// If ceiling2 is given, the box value is scaled with (x * ceiling2) / 256. Otherwise,
	// if it is wilderness, the scale value is 192.

	// In the voxel data, the most significant byte of raised platforms contains the thickness
	// and height indices, organized as 0x0tttthhh.

	// 'a': interiors/dungeons
	// 'b': cities
	// 'c': wilderness

	// Raised platform Y heights (Box1A/Box1B/Box1C) and thicknesses (Box2A/Box2B).
	uint16_t boxArrays[56];

	// Unscaled copy of previous array to restore global variables with in the original game.
	uint16_t boxArraysCopy[56];

	// Raised platform texture mapping values. Box4 is incorrectly accessed in the original game as if it were a Box3C.
	uint16_t box3a[8], box3b[8];

	// Number of texels tall a 64x64 texture is rendered as, also used with calculation for starting row in texture.
	uint16_t box4[8];

	Span<uint16_t> heightsInterior, heightsCity, heightsWild;
	Span<uint16_t> thicknessesInterior, thicknessesCity, thicknessesWild;
	Span<uint16_t> texMappingInterior, texMappingCity, texMappingWild;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);

	// Unknown
	int getTextureMappingValueA(MapType mapType, int heightIndex) const;

	// Determines the starting row in a texture?
	int getTextureMappingValueB(int thicknessIndex, int textureMappingValueA) const;
};

struct ExeDataStatus
{
	// Status pop-up text (with %s/%d tokens).
	std::string popUp;

	// Weekday/day/month/year text (with %s tokens), used in pop-up.
	std::string date;

	std::string fortify; // With %s token.
	std::string disease; // With %s token.
	std::string effect; // With %s token.
	std::string effectsList[23]; // Healthy, diseased, etc..

	std::string keyNames[12];
	std::string keyPickedUp;
	std::string doorUnlockedWithKey;
	std::string lockDifficultyMessages[14];

	std::string staminaExhaustedRecover;
	std::string staminaExhaustedDeath;
	std::string staminaDrowning;

	std::string enemyCorpseEmptyInventory;
	std::string enemyCorpseGold;
	std::string citizenCorpseGold;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataTravel
{
	// Location format texts when clicking on a location on a province map.
	// Each one is phrased a little differently:
	// - <dungeon> in <province> Province
	// - The <center province city> in the <center province>
	// - The <city type> of <city name> in <province> Province
	std::string locationFormatTexts[3];

	// Description for how many days traveling will take based on the current weather,
	// split into two strings for some reason.
	std::string dayPrediction[2];

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
	std::string staffDungeonSplashes[8];

	// Province indices into the staff dungeon splash filenames.
	uint8_t staffDungeonSplashIndices[8];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataUI
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
	uint16_t followerPortraitPositions[16];

	// Armor class numbers on character sheet. In X, Y format.
	uint16_t maleArmorClassPositions[14], femaleArmorClassPositions[14];

	// Thirty palette indices to translate some races' helmet skin colors with.
	// "race#" points to the race in "province #". Race 7 is a special case; they
	// use "xLIZn.IMG" instead, where 'x' is either 'M' or 'F', and 'n' is the
	// armor type (plate=0, etc.).
	uint8_t helmetPaletteIndices[30], race1HelmetPaletteValues[30], race3HelmetPaletteValues[30], race4HelmetPaletteValues[30];

	// Displayed when pressing F2.
	std::string currentWorldPosition;

	// Displayed when clicking an entity.
	std::string inspectedEntityName;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataWeather
{
	int16_t fogTxtSampleHelper[24];
	uint8_t thunderstormFlashColors[3];

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

struct ExeDataWilderness
{
	// .RMD index lists for each type of block (normal, village, dungeon, tavern, and temple).
	// Each list starts with the number of elements.
	Buffer<uint8_t> normalBlocks, villageBlocks, dungeonBlocks, tavernBlocks, templeBlocks;

	bool init(Span<const std::byte> exeBytes, const KeyValueFile &keyValueFile);
};

// A view into various ranges of bytes in the original Arena executable A.EXE or ACD.EXE.
struct ExeData
{
	static const std::string FLOPPY_VERSION_EXE_FILENAME;
	static const std::string FLOPPY_VERSION_MAP_FILENAME;
	static const std::string CD_VERSION_EXE_FILENAME;
	static const std::string CD_VERSION_MAP_FILENAME;

	bool isFloppyVersion;
	ExeDataCalendar calendar;
	ExeDataCharacterClasses charClasses;
	ExeDataCharacterCreation charCreation;
	ExeDataCityGeneration cityGen;
	ExeDataEntities entities;
	ExeDataEquipment equipment;
	ExeDataItems items;
	ExeDataLight light;
	ExeDataLocations locations;
	ExeDataLogbook logbook;
	ExeDataMath math;
	ExeDataMeta meta;
	ExeDataQuests quests;
	ExeDataRaces races;
	ExeDataStatus status;
	ExeDataTravel travel;
	ExeDataUI ui;
	ExeDataRaisedPlatforms raisedPlatforms;
	ExeDataWeather weather;
	ExeDataWilderness wild;

	ExeData();

	bool init(bool floppyVersion);
};

#endif
