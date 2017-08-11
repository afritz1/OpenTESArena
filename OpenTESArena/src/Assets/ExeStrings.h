#ifndef EXE_STRINGS_H
#define EXE_STRINGS_H

#include <vector>

// This file has various offsets and sizes for strings in the decompressed A.EXE.

// I'm not sure how I want to store these in the long run, so this is a rough draft
// layout for now.

namespace ExeStrings
{
// Character creation.
const std::pair<int, int> ChooseClassCreation(0x00035a80, 37);
const std::pair<int, int> ChooseClassCreationGenerate(0x0003f637, 8);
const std::pair<int, int> ChooseClassCreationSelect(0x0003f641, 6);
const std::pair<int, int> ClassQuestionsIntro(0x00035aa7, 175);
const std::pair<int, int> SuggestedRace(0x00035bb1, 75);
const std::pair<int, int> ChooseClassList(0x0003f61a, 19);
const std::pair<int, int> ChooseName(0x00035b58, 26);
const std::pair<int, int> ChooseGender(0x00035b74, 20);
const std::pair<int, int> ChooseGenderMale(0x0003f652, 4);
const std::pair<int, int> ChooseGenderFemale(0x0003f658, 6);
const std::pair<int, int> ChooseRace(0x00035b8a, 37);
const std::pair<int, int> ConfirmRace(0x00035bff, 74);
const std::pair<int, int> FinalRaceMessage(0x00035ce0, 67);
const std::pair<int, int> DistributeClassPoints(0x00035d25, 93);
const std::pair<int, int> ChooseAppearance(0x00035d84, 174);

// Class names. Unordered.
const std::vector<std::pair<int, int>> MageClassNames =
{
	{ 0x0003e15e, 4 },
	{ 0x0003e163, 10 },
	{ 0x0003e16e, 10 },
	{ 0x0003e179, 8 },
	{ 0x0003e182, 6 },
	{ 0x0003e189, 10 }
};

const std::vector<std::pair<int, int>> ThiefClassNames =
{
	{ 0x0003e194, 4 },
	{ 0x0003e199, 7 },
	{ 0x0003e1a1, 5 },
	{ 0x0003e1a7, 7 },
	{ 0x0003e1af, 5 },
	{ 0x0003e1b5, 8 }
};

const std::vector<std::pair<int, int>> WarriorClassNames =
{
	{ 0x0003e1be, 4 },
	{ 0x0003e1c3, 6 },
	{ 0x0003e1ca, 6 },
	{ 0x0003e1d1, 9 },
	{ 0x0003e1db, 7 },
	{ 0x0003e1e3, 6 }
};

// Province names, ordered by appearance on the world map reading from left to right, 
// with the center province last.
const std::vector<std::pair<int, int>> ProvinceNames =
{
	{ 0x000392f8, 9 },
	{ 0x0003935a, 10 },
	{ 0x000393bc, 6 },
	{ 0x0003941e, 9 },
	{ 0x00039480, 14 },
	{ 0x000394e2, 9 },
	{ 0x00039544, 7 },
	{ 0x000395a6, 11 },
	{ 0x00039608, 17 }
};

// Province .IMG backgrounds, ordered the same as province names.
const std::vector<std::pair<int, int>> ProvinceIMGFilenames =
{
	{ 0x0003fdfa, 12 },
	{ 0x0003fe07, 12 },
	{ 0x0003fe14, 10 },
	{ 0x0003fe1f, 12 },
	{ 0x0003fe2c, 12 },
	{ 0x0003fe39, 12 },
	{ 0x0003fe46, 11 },
	{ 0x0003fe52, 12 },
	{ 0x0003fe5f, 12 }
};

// Race names. Ordered the same as provinces.
const std::vector<std::pair<int, int>> RaceNamesSingular =
{
	{ 0x0003e290, 6 },
	{ 0x0003e297, 8 },
	{ 0x0003e2a0, 4 },
	{ 0x0003e2a5, 8 },
	{ 0x0003e2ae, 8 },
	{ 0x0003e2b7, 8 },
	{ 0x0003e2c0, 7 },
	{ 0x0003e2c8, 8 }
};

const std::vector<std::pair<int, int>> RaceNamesPlural =
{
	{ 0x0003e245, 7 },
	{ 0x0003e24d, 9 },
	{ 0x0003e257, 5 },
	{ 0x0003e25d, 10 },
	{ 0x0003e268, 10 },
	{ 0x0003e273, 10 },
	{ 0x0003e27e, 7 },
	{ 0x0003e286, 9 }
};

// Logbook.
const std::pair<int, int> LogbookIsEmpty(0x00042f45, 22);

// Time of day strings.
const std::vector<std::pair<int, int>> TimeOfDayStrings =
{
	{ 0x00040529, 13 },
	{ 0x00040537, 14 },
	{ 0x00040546, 4 },
	{ 0x0004054b, 16 },
	{ 0x0004055c, 14 },
	{ 0x0004056b, 8 },
	{ 0x00040574, 8 }
};

// Weekday names.
const std::vector<std::pair<int, int>> WeekdayNames =
{
	{ 0x0003e92a, 7 },
	{ 0x0003e932, 6 },
	{ 0x0003e939, 6 },
	{ 0x0003e940, 6 },
	{ 0x0003e947, 6 },
	{ 0x0003e94e, 7 },
	{ 0x0003e956, 6 }
};

// Month names.
const std::vector<std::pair<int, int>> MonthNames =
{
	{ 0x0003e894, 12 },
	{ 0x0003e8a1, 10 },
	{ 0x0003e8ac, 10 },
	{ 0x0003e8b7, 11 },
	{ 0x0003e8c3, 11 },
	{ 0x0003e8cf, 8 },
	{ 0x0003e8d8, 12 },
	{ 0x0003e8e5, 9 },
	{ 0x0003e8ef, 10 },
	{ 0x0003e8fa, 9 },
	{ 0x0003e904, 10 },
	{ 0x0003e90f, 12 }
};

// Creature names ("Rat", "Goblin", etc.). Each creature type could simply use its index 
// in this array as its identifier, much like with provinces.
const std::vector<std::pair<int, int>> CreatureNames =
{
	{ 0x00036bbe, 3 },
	{ 0x00036bc2, 6 },
	{ 0x00036bc9, 10 },
	{ 0x00036bd4, 4 },
	{ 0x00036bd9, 9 },
	{ 0x00036be3, 3 },
	{ 0x00036be7, 8 },
	{ 0x00036bf0, 8 },
	{ 0x00036bf9, 6 },
	{ 0x00036c00, 5 },
	{ 0x00036c06, 10 },
	{ 0x00036c11, 5 },
	{ 0x00036c17, 6 },
	{ 0x00036c1e, 5 },
	{ 0x00036c24, 6 },
	{ 0x00036c2b, 10 },
	{ 0x00036c36, 9 },
	{ 0x00036c40, 11 },
	{ 0x00036c4c, 10 },
	{ 0x00036c57, 11 },
	{ 0x00036c63, 6 },
	{ 0x00036c6a, 7 },
	{ 0x00036c72, 4 }
};

// Creature .CFA filenames. These are ordered the same as creature names, and there is
// an extra entry at the end for the final boss. Replace '@' with a number from 1 to 6 
// indicating which .CFA file to fetch for angle-relative animations.
const std::vector<std::pair<int, int>> CreatureAnimations =
{
	{ 0x0003e4fb, 8 },
	{ 0x0003e504, 11, },
	{ 0x0003e510, 11 },
	{ 0x0003e51c, 10 },
	{ 0x0003e527, 11 },
	{ 0x0003e533, 8 },
	{ 0x0003e53c, 10 },
	{ 0x0003e547, 8 },
	{ 0x0003e550, 11 },
	{ 0x0003e55c, 10 },
	{ 0x0003e567, 10 },
	{ 0x0003e572, 10 },
	{ 0x0003e57d, 11 },
	{ 0x0003e589, 10 },
	{ 0x0003e594, 11 },
	{ 0x0003e5a0, 8 },
	{ 0x0003e5a9, 8 },
	{ 0x0003e5b2, 10 },
	{ 0x0003e5bd, 9 },
	{ 0x0003e5c7, 11 },
	{ 0x0003e5d3, 11 },
	{ 0x0003e5df, 12 },
	{ 0x0003e5ec, 9 },
	{ 0x0003e5f6, 11 }
};

// Random male citizen .CFA filenames. Replace '@' with a number from 1 to 5.
const std::vector<std::pair<int, int>> MaleCitizenAnimations =
{
	{ 0x0004186a, 12 }, // Winter.
	{ 0x00041877, 12 }, // Desert.
	{ 0x00041884, 12 } // Temperate.
};

// Random female citizen .CFA filenames. Replace '@' with a number from 1 to 5.
const std::vector<std::pair<int, int>> FemaleCitizenAnimations =
{
	{ 0x00041891, 11 }, // Temperate.
	{ 0x0004189d, 12 }, // Desert.
	{ 0x000418aa, 12 } // Winter.
};

// Filename chunks for sprites with variable weapons, etc., to be combined with the 
// .CFA filenames containing three X's (walk, attack, bow).
const std::vector<std::pair<int, int>> CFAFilenameChunks =
{
	{ 0x00041ff0, 3 }, // Plate.
	{ 0x00041ff4, 3 }, // Chain.
	{ 0x00041ff8, 3 }, // Leather.
	{ 0x00041ffc, 3 }, // Unarmored.
	{ 0x00042000, 3 }, // Mage.
	{ 0x00042004, 3 }, // Monk.
	{ 0x00042008, 3 } // Barbarian.
};

// Filename templates to be used with the .CFA filename chunks. Replace "0@" with 
// either 0 or 1 for the gender, and 1 to 5 depending on the kind of animation (i.e., 
// attacks are only forward, so they don't have 2 through 5). Replace "XXX" with one 
// of the .CFA filename chunks. Not every character has every combination (i.e., 
// barbarians don't have a bow shoot animation, and there is no female plate).
const std::vector<std::pair<int, int>> CFAFilenameTemplates =
{
	{ 0x0004200c, 12 }, // Walk.
	{ 0x00042019, 12 }, // Attack.
	{ 0x00042026, 12 } // Bow.
};

// A few premade human .CFA animations with some weapons. Some of these can't be 
// created from pairing a .CFA template with a .CFA chunk because there aren't the 
// right selection of templates available (I think... because it's kind of messy).
const std::vector<std::pair<int, int>> CFAHumansWithWeaponAnimations =
{
	{ 0x00042033, 12 }, // Mage + sword.
	{ 0x00042040, 12 }, // Mage + staff.
	{ 0x0004204d, 12 }, // Mage spell.
	{ 0x0004205a, 12 } // Monk kick.
};

// Human .CFA weapons by themselves, presumably for combining with empty-handed human
// animations.
const std::vector<std::pair<int, int>> CFAWeaponAnimations =
{
	{ 0x00042067, 11 }, // Sword.
	{ 0x00042073, 9 }, // Axe.
	{ 0x0004207d, 10 }, // Mace.
	{ 0x00042088, 12 }, // "P" sword.
	{ 0x00042095, 10 }, // "P" axe.
	{ 0x000420a0, 11 }, // "P" mace.
	{ 0x000420ac, 12 }, // "B" sword.
	{ 0x000420b9, 10 }, // "B" axe.
	{ 0x000420c4, 11 }, // "B" mace.
	{ 0x000420d0, 12 }, // Shield.
	{ 0x000420dd, 11 }, // "P" shield.
	{ 0x000420e9, 11 } // "B" shield.
};

}

#endif
