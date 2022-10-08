#ifndef VOXEL_INSTANCE_H
#define VOXEL_INSTANCE_H

#include "VoxelUtils.h"

// Values for a voxel changing over time or being uniquely different in some way.

enum class VoxelFacing2D;
enum class VoxelFacing3D;

class VoxelInstance
{
public:
	enum class Type { Chasm };

	// @todo: break VoxelInstance into more pieces because each type is very different from each other system-wise.
	// Chunk should have chasmInsts, openDoorInsts, fadingInsts, etc..
	// - Chasm: turns geometry on/off
	// - OpenDoor: transforms geometry position/rotation
	// - Fading: pixel shader variable
	// - Trigger: one-shot lore text presentation

	// @todo: maybe a BashState?

	class ChasmState // @todo: is this necessary? Can't we just query adjacent voxels' VoxelMeshDefinitions for enablesNeighborGeometry?
	{
	private:
		// Visible chasm faces.
		bool north, east, south, west;
	public:
		void init(bool north, bool east, bool south, bool west);

		bool getNorth() const;
		bool getEast() const;
		bool getSouth() const;
		bool getWest() const;
		bool faceIsVisible(VoxelFacing3D facing) const;
		bool faceIsVisible(VoxelFacing2D facing) const;
		int getFaceCount() const;
	};
private:
	SNInt x;
	int y;
	WEInt z;
	Type type;

	union
	{
		ChasmState chasm;
	};

	void init(SNInt x, int y, WEInt z, Type type);
public:
	VoxelInstance();

	static VoxelInstance makeChasm(SNInt x, int y, WEInt z, bool north, bool east,
		bool south, bool west);

	SNInt getX() const;
	int getY() const;
	WEInt getZ() const;
	Type getType() const;
	ChasmState &getChasmState();
	const ChasmState &getChasmState() const;

	// Returns whether the voxel instance is worth keeping alive because it has unique data active.
	bool hasRelevantState() const;
};

#endif
