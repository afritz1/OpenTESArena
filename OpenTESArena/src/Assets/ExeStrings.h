#ifndef EXE_STRINGS_H
#define EXE_STRINGS_H

#include <vector>

// This file has various offsets and sizes for strings in the decompressed A.EXE.

// I'm not sure how I want to store these in the long run, so this is a rough draft
// layout for now.

namespace ExeStrings
{
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
}

#endif
