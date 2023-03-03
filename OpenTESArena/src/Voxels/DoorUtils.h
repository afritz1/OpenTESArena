#ifndef DOOR_UTILS_H
#define DOOR_UTILS_H

#include <array>

#include "VoxelFacing2D.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../World/ArenaMeshUtils.h"

namespace DoorUtils
{
	static constexpr int FACE_COUNT = ArenaMeshUtils::GetUniqueFaceCount(ArenaTypes::VoxelType::Door);

	constexpr std::array<VoxelFacing2D, FACE_COUNT> Facings =
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
	constexpr std::array<Radians, FACE_COUNT> BaseAngles =
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
	const std::array<Double3, FACE_COUNT> SwingingHingeOffsets =
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
}

#endif
