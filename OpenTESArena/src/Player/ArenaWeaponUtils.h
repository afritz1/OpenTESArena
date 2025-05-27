#ifndef ARENA_WEAPON_UTILS_H
#define ARENA_WEAPON_UTILS_H

#include <string>
#include <vector>

#include "WeaponAnimationUtils.h"

namespace ArenaWeaponUtils
{
	constexpr int FistsFilenameIndex = 7;
	constexpr int FilenameIndices[] =
	{
		0, // Staff
		1, // Dagger
		1, // Shortsword
		1, // Broadsword
		1, // Saber
		1, // Longsword
		1, // Claymore
		1, // Tanto
		1, // Wakizashi
		1, // Katana
		1, // Dai-katana
		2, // Mace
		3, // Flail
		4, // War hammer
		5, // War axe
		5, // Battle axe
		6, // Short bow
		6  // Long bow
	};

	constexpr int MeleeWeaponTypeCount = 16;
	constexpr int RangedWeaponTypeCount = 2;

	struct AnimationStateInfo
	{
		std::string name;
		std::vector<int> frames;
		double timeScale;
	};

	constexpr double FRAMES_PER_SECOND = 16.0;
	constexpr double DEFAULT_TIME_SCALE = 1.0;

	const AnimationStateInfo MeleeAnimationStateInfos[] =
	{
		{ WeaponAnimationUtils::STATE_SHEATHED, { }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_UNSHEATHING, { 30, 31, 32 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_SHEATHING, { 32, 31, 30 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_IDLE, { 32 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_FORWARD, { 25, 26, 27, 28, 29 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_DOWN, { 0, 1, 2, 3, 4 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_RIGHT, { 15, 16, 17, 18, 19 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_LEFT, { 10, 11, 12, 13, 14 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_DOWN_RIGHT, { 20, 21, 22, 23, 24 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_DOWN_LEFT, { 5, 6, 7, 8, 9 }, DEFAULT_TIME_SCALE }
	};

	const AnimationStateInfo FistsAnimationStateInfos[] =
	{
		{ WeaponAnimationUtils::STATE_SHEATHED, { }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_UNSHEATHING, { 10, 11, 12 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_SHEATHING, { 12, 11, 10 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_IDLE, { 12 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_FORWARD, { 5, 6, 7, 8, 9 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_DOWN, { 0, 1, 2, 3, 4 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_RIGHT, { 5, 6, 7, 8, 9 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_LEFT, { 0, 1, 2, 3, 4 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_DOWN_RIGHT, { 5, 6, 7, 8, 9 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_DOWN_LEFT, { 0, 1, 2, 3, 4 }, DEFAULT_TIME_SCALE }
	};

	const AnimationStateInfo BowAnimationStateInfos[] =
	{
		{ WeaponAnimationUtils::STATE_SHEATHED, { }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_UNSHEATHING, { 0 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_SHEATHING, { 0 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_IDLE, { 0 }, DEFAULT_TIME_SCALE },
		{ WeaponAnimationUtils::STATE_FIRING, { 1 }, 1.0 / 7.0 } // This frame lasts longer
	};
}

#endif
