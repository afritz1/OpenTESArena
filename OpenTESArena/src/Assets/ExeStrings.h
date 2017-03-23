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

// Province names.
// - Ordered by appearance on the world map reading from top left to bottom right, 
//   with the center province last.
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
}

#endif
