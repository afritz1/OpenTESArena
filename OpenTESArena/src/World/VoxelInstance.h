#ifndef VOXEL_INSTANCE_H
#define VOXEL_INSTANCE_H

#include "VoxelUtils.h"

// Values for a voxel changing over time or being uniquely different in some way.

enum class VoxelFacing2D;
enum class VoxelFacing3D;

class VoxelInstance
{
public:
	enum class Type { Chasm, OpenDoor, Fading, Trigger };

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

	class DoorState
	{
	public:
		enum class StateType { Closed, Opening, Open, Closing };
	private:
		double speed;
		double percentOpen;
		StateType stateType;
	public:
		void init(double speed, double percentOpen, StateType stateType);

		double getSpeed() const;
		double getPercentOpen() const;
		StateType getStateType() const;

		void setStateType(StateType stateType);
		void update(double dt);
	};

	class FadeState
	{
	private:
		double speed;
		double percentFaded;
	public:
		void init(double speed, double percentFaded);

		double getSpeed() const;
		double getPercentFaded() const;
		bool isDoneFading() const;

		void update(double dt);
	};

	class TriggerState
	{
	private:
		bool triggered;
	public:
		void init(bool triggered);

		bool isTriggered() const;
	};
private:
	SNInt x;
	int y;
	WEInt z;
	Type type;

	union
	{
		ChasmState chasm;
		DoorState door;
		FadeState fade;
		TriggerState trigger;
	};

	void init(SNInt x, int y, WEInt z, Type type);
public:
	VoxelInstance();

	static VoxelInstance makeChasm(SNInt x, int y, WEInt z, bool north, bool east,
		bool south, bool west);

	static VoxelInstance makeDoor(SNInt x, int y, WEInt z, double speed, double percentOpen,
		DoorState::StateType stateType);
	
	// Default to opening (so it isn't cleared on the first frame).
	static VoxelInstance makeDoor(SNInt x, int y, WEInt z, double speed);

	static VoxelInstance makeFading(SNInt x, int y, WEInt z, double speed, double percentFaded);

	// Default to beginning fade.
	static VoxelInstance makeFading(SNInt x, int y, WEInt z, double speed);

	static VoxelInstance makeTrigger(SNInt x, int y, WEInt z, bool triggered);

	SNInt getX() const;
	int getY() const;
	WEInt getZ() const;
	Type getType() const;
	ChasmState &getChasmState();
	const ChasmState &getChasmState() const;
	DoorState &getDoorState();
	const DoorState &getDoorState() const;
	FadeState &getFadeState();
	const FadeState &getFadeState() const;
	TriggerState &getTriggerState();
	const TriggerState &getTriggerState() const;

	// Returns whether the voxel instance is worth keeping alive because it has unique data active.
	bool hasRelevantState() const;

	void update(double dt);
};

#endif
