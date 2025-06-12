#ifndef VOXEL_DOOR_UTILS_H
#define VOXEL_DOOR_UTILS_H

#include "VoxelFacing.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../World/ArenaMeshUtils.h"

struct VoxelChunk;

namespace VoxelDoorUtils
{
	static constexpr int FACE_COUNT = ArenaMeshUtils::GetUniqueFaceCount(ArenaVoxelType::Door);

	constexpr VoxelFacing2D Facings[] =
	{
		// X=0
		VoxelFacing2D::NegativeX,
		// X=1
		VoxelFacing2D::PositiveX,
		// Z=0
		VoxelFacing2D::NegativeZ,
		// Z=1
		VoxelFacing2D::PositiveZ
	};

	// Angle away from default face's orientation when closed.
	constexpr Radians BaseAngles[] =
	{
		// X=0
		0.0,
		// X=1
		Constants::Pi,
		// Z=0
		Constants::HalfPi,
		// Z=1
		Constants::HalfPi * 3.0
	};

	// Distance of the swinging door hinge from the voxel origin.
	const Double3 SwingingHingeOffsets[] =
	{
		// X=0
		Double3::Zero,
		// X=1
		Double3::UnitX + Double3::UnitZ,
		// Z=0
		Double3::UnitX,
		// Z=1
		Double3::UnitZ
	};

	double getAnimPercentOrZero(SNInt x, int y, WEInt z, const VoxelChunk &voxelChunk);
	Radians getSwingingRotationRadians(Radians baseRadians, double animPercent);
	double getAnimatedTexCoordPercent(double animPercent);
	double getAnimatedScaleAmount(double texCoordPercent);
}

#endif
