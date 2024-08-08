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

void RayCastHit::initVoxel(double t, const CoordDouble3 &coord, const VoxelInt3 &voxel, VoxelFacing3D facing)
{
	this->t = t;
	this->coord = coord;

	this->type = RayCastHitType::Voxel;
	this->voxelHit.voxel = voxel;
	this->voxelHit.facing = facing;
}

void RayCastHit::initEntity(double t, const CoordDouble3 &coord, EntityInstanceID id)
{
	this->t = t;
	this->coord = coord;

	this->type = RayCastHitType::Entity;
	this->entityHit.id = id;
}
