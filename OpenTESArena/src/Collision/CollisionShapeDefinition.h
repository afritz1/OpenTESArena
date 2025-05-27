#ifndef COLLISION_SHAPE_DEFINITION_H
#define COLLISION_SHAPE_DEFINITION_H

#include "../Math/MathUtils.h"

enum class CollisionShapeType
{
	Box, // Voxels inc. air (for sound/lore triggers)
	Capsule // Entities inc. projectiles
};

struct CollisionBoxShapeDefinition
{
	double width, height, depth;
	double yOffset; // Elevation above bottom of voxel.
	Radians yRotation; // For diagonal walls.

	void init(double width, double height, double depth, double yOffset, Radians yRotation);
};

struct CollisionCapsuleShapeDefinition
{
	double radius, middleHeight;
	double totalHeight; // (radius * 2) + middleHeight

	void init(double radius, double middleHeight);
};

struct CollisionShapeDefinition
{
	CollisionShapeType type;

	union
	{
		CollisionBoxShapeDefinition box;
		CollisionCapsuleShapeDefinition capsule;
	};

	CollisionShapeDefinition();

	void initBox(double width, double height, double depth, double yOffset, Radians yRotation);
	void initCapsule(double radius, double middleHeight);
	void initCapsuleAsSphere(double radius);
};

#endif
