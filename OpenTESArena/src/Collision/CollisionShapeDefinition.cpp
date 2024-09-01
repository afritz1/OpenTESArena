#include "CollisionShapeDefinition.h"

#include "components/debug/Debug.h"

void CollisionBoxShapeDefinition::init(double width, double height, double depth, double yOffset, Radians yRotation)
{
	DebugAssert(width > 0.0);
	DebugAssert(height > 0.0);
	DebugAssert(depth > 0.0);
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->yOffset = yOffset;
	this->yRotation = yRotation;
}

void CollisionCapsuleShapeDefinition::init(double radius, double middleHeight)
{
	DebugAssert(radius > 0.0);
	DebugAssert(middleHeight >= 0.0); // Can be 0 for spheres
	this->radius = radius;
	this->middleHeight = middleHeight;
	this->totalHeight = (radius + radius) + middleHeight;
}

CollisionShapeDefinition::CollisionShapeDefinition()
{
	this->type = static_cast<CollisionShapeType>(-1);
}

void CollisionShapeDefinition::initBox(double width, double height, double depth, double yOffset, Radians yRotation)
{
	this->type = CollisionShapeType::Box;
	this->box.init(width, height, depth, yOffset, yRotation);
}

void CollisionShapeDefinition::initCapsule(double radius, double middleHeight)
{
	this->type = CollisionShapeType::Capsule;
	this->capsule.init(radius, middleHeight);
}

void CollisionShapeDefinition::initCapsuleAsSphere(double radius)
{
	this->initCapsule(radius, 0.0);
}
