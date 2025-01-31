#ifndef ARENA_PLAYER_UTILS_H
#define ARENA_PLAYER_UTILS_H

namespace ArenaPlayerUtils
{
	int getBaseSpeed(int speedAttribute, int encumbranceMod);
	int getMoveSpeed(int baseSpeed);
	int getTurnSpeed(int baseSpeed);

	constexpr int AccelerationRate = 32;
	constexpr int AccelerationMax = 256;
	constexpr int DecelerationRate = 64;

	constexpr int ChasmHeightDeltaDryChasm = -80;
	constexpr int ChasmHeightSwimmingInterior = -25;
	constexpr int ChasmHeightSwimmingCity = -50;
	constexpr int ChasmHeightSwimmingWild = -10;
	constexpr int ChasmHeightRowBoatInterior = -10;
	constexpr int ChasmHeightRowBoatWild = -1;

	constexpr int ChasmMagnetUnitsPerFrame = 16;
	int getChasmFallSpeed(int frame);

	constexpr int ChasmClimbingUnitsPerFrame = 6;

	constexpr int JumpFrameCount = 10;
	constexpr int JumpFrameCountAcrobat = JumpFrameCount * 2;
	constexpr int JumpDisallowedCameraHeightUnits = 60; // Or in voxel column with any chasm.
	constexpr int StandingJumpForwardUnits = 30; // (dist*pc.attr[STRENGTH])/128
	int getJumpUnitsPerFrame(int frame);

}

#endif
