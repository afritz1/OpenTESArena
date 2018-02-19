#ifndef EXE_STRINGS_H
#define EXE_STRINGS_H

#include <string>
#include <unordered_map>
#include <vector>

// The ExeStrings class reads the text of an Arena executable and fetches various strings.

// The key strings are intended to work between both A.EXE and ACD.EXE, although
// the mapped offsets and sizes in the key-value map will most likely be different.

enum class ExeStringKey
{
	// Character creation.
	ChooseClassCreation,
	ChooseClassCreationGenerate,
	ChooseClassCreationSelect,
	ClassQuestionsIntro,
	SuggestedRace,
	ChooseClassList,
	ChooseName,
	ChooseGender,
	ChooseGenderMale,
	ChooseGenderFemale,
	ChooseRace,
	ConfirmRace,
	ConfirmedRace1,
	ConfirmedRace2,
	ConfirmedRace3,
	ConfirmedRace4,
	DistributeClassPoints,
	ChooseAttributesChoice,
	ChooseAttributesSave,
	ChooseAttributesReroll,
	ChooseAppearance,

	// Character class names (ordered by: mages, thieves, warriors).
	CharacterClassNames,

	// Character class data.
	AllowedArmors,
	AllowedShields,
	AllowedWeapons,
	ClassAttributes,
	ClassNumberToClassID,
	ClassInitialExperienceCap,
	HealthDice,
	LockpickingDivisors,

	// Province names and backgrounds.
	CharCreationProvinceNames,
	ProvinceNames,
	ProvinceIMGFilenames,

	// Locations.
	StartDungeonName,
	CityTemplateFilenames,

	// Race names.
	RaceNamesSingular,
	RaceNamesPlural,

	// Logbook.
	LogbookIsEmpty,

	// Times of day.
	TimesOfDay,

	// Calendar names.
	WeekdayNames,
	MonthNames,

	// Creature names and animation filenames.
	CreatureNames,
	CreatureAnimations,

	// Random NPC .CFA filenames.
	MaleCitizenAnimations,
	FemaleCitizenAnimations,

	// Filename chunks for sprites with variable weapons.
	CFAFilenameChunks,

	// Filename templates for use with .CFA chunks.
	CFAFilenameTemplates,

	// Premade human .CFA animations with some weapons.
	CFAHumansWithWeaponAnimations,

	// Human .CFA weapons by themselves (for combining with empty-handed animations).
	CFAWeaponAnimations,

	// Body part names.
	BodyPartNames,

	// Equipment-related names.
	ArmorNames,
	ShieldNames,
	WeaponNames,
	MetalNames,
	WeaponAnimationFilenames
};

namespace std
{
	// Hash function for ExeStringKey, since GCC doesn't support enum classes as 
	// unordered_map keys until C++14.
	template <>
	struct hash<ExeStringKey>
	{
		size_t operator()(const ExeStringKey &k) const
		{
			return static_cast<size_t>(k);
		}
	};
}

class ExeStrings
{
private:
	std::unordered_map<ExeStringKey, std::string> strings;
	std::unordered_map<ExeStringKey, std::vector<std::string>> stringLists;
public:
	ExeStrings(const std::string &exeText, const std::string &keyValueMapFilename);
	~ExeStrings();

	// Gets a string value, given an ExeStringKey. Intended only for key-value pairs
	// whose value is a single offset + size pair.
	const std::string &get(ExeStringKey key) const;

	// Gets a string list, given an ExeStringKey. Intended only for key-value pairs
	// whose value is a list of offset + size pairs.
	const std::vector<std::string> &getList(ExeStringKey key) const;
};

#endif
