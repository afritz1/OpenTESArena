#include "EntityVisibilityState.h"

EntityVisibilityState2D::EntityVisibilityState2D()
	: flatPosition(ChunkInt2::Zero, VoxelDouble2::Zero)
{
	this->entityInstID = -1;
	this->stateIndex = -1;
	this->angleIndex = -1;
	this->keyframeIndex = -1;
}

void EntityVisibilityState2D::init(EntityInstanceID entityInstID, const CoordDouble2 &flatPosition,
	int stateIndex, int angleIndex, int keyframeIndex)
{
	this->entityInstID = entityInstID;
	this->flatPosition = flatPosition;
	this->stateIndex = stateIndex;
	this->angleIndex = angleIndex;
	this->keyframeIndex = keyframeIndex;
}

EntityVisibilityState3D::EntityVisibilityState3D()
	: flatPosition(ChunkInt2::Zero, VoxelDouble3::Zero)
{
	this->entityInstID = -1;
	this->stateIndex = -1;
	this->angleIndex = -1;
	this->keyframeIndex = -1;
}

void EntityVisibilityState3D::init(EntityInstanceID entityInstID, const CoordDouble3 &flatPosition,
	int stateIndex, int angleIndex, int keyframeIndex)
{
	this->entityInstID = entityInstID;
	this->flatPosition = flatPosition;
	this->stateIndex = stateIndex;
	this->angleIndex = angleIndex;
	this->keyframeIndex = keyframeIndex;
}
