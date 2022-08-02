#ifndef ARENA_CHASM_UTILS_H
#define ARENA_CHASM_UTILS_H

#include "../Assets/ArenaTypes.h"

namespace ArenaChasmUtils
{
	bool allowsSwimming(ArenaTypes::ChasmType chasmType);
	bool isDamaging(ArenaTypes::ChasmType chasmType);
}

#endif
