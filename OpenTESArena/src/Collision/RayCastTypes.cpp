#include "RayCastTypes.h"

RayCastVoxelHit::RayCastVoxelHit()
{
	this->facing = static_cast<VoxelFacing3D>(-1);
}

RayCastHit::RayCastHit()
{
	this->t = 0.0;
	this->type = static_cast<RayCastHitType>(-1);
}

RayCastEntityHit::RayCastEntityHit()
{
	this->id = -1;
}

void RayCastHit::initVoxel(double t, const WorldDouble3 &worldPoint, const CoordInt3 &voxelCoord, VoxelFacing3D facing)
{
	this->t = t;
	this->worldPoint = worldPoint;

	this->type = RayCastHitType::Voxel;
	this->voxelHit.voxelCoord = voxelCoord;
	this->voxelHit.facing = facing;
}

void RayCastHit::initEntity(double t, const WorldDouble3 &worldPoint, EntityInstanceID id)
{
	this->t = t;
	this->worldPoint = worldPoint;

	this->type = RayCastHitType::Entity;
	this->entityHit.id = id;
}
