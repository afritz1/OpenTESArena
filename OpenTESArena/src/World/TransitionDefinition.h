#ifndef TRANSITION_DEFINITION_H
#define TRANSITION_DEFINITION_H

// @todo: share with both voxels and entities; probably delete EntityDefinition
// variant of this class.

class TransitionDefinition
{
public:
	enum class Type
	{
		CityGate, // Swaps to city or wilderness, whichever is inactive.
		EnterInterior,
		ExitInterior,
		LevelChange
	};

	struct LevelChangeDef
	{
		bool isLevelUp;

		void init(bool isLevelUp);
	};
private:
	Type type;

	union
	{
		LevelChangeDef levelChange;
	};

	void init(Type type);
public:
	TransitionDefinition();

	void initCityGate();
	void initInteriorEntrance();
	void initInteriorExit();
	void initLevelChange(bool isLevelUp);

	Type getType() const;
	const LevelChangeDef &getLevelChange() const;
};

#endif
