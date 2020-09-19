#ifndef VOXEL_INSTANCE_H
#define VOXEL_INSTANCE_H

#include "VoxelUtils.h"

// Values for a voxel changing over time or being uniquely different in some way.

class VoxelInstance
{
public:
	enum class Type { OpenDoor, Fading };

	// @todo: move chasm facings here from VoxelDefinition
	// @todo: maybe a BashState?

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
private:
	SNInt x;
	int y;
	WEInt z;
	Type type;

	union
	{
		DoorState door;
		FadeState fade;
	};

	void init(SNInt x, int y, WEInt z, Type type);
public:
	static VoxelInstance makeDoor(SNInt x, int y, WEInt z, double speed, double percentOpen,
		DoorState::StateType stateType);
	
	// Default to opening (so it isn't cleared on the first frame).
	static VoxelInstance makeDoor(SNInt x, int y, WEInt z, double speed);

	static VoxelInstance makeFading(SNInt x, int y, WEInt z, double speed, double percentFaded);

	// Default to beginning fade.
	static VoxelInstance makeFading(SNInt x, int y, WEInt z, double speed);

	SNInt getX() const;
	int getY() const;
	WEInt getZ() const;
	Type getType() const;
	DoorState &getDoorState();
	const DoorState &getDoorState() const;
	FadeState &getFadeState();
	const FadeState &getFadeState() const;

	void update(double dt);
};

#endif
