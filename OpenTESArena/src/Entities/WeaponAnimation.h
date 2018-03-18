#ifndef WEAPON_ANIMATION_H
#define WEAPON_ANIMATION_H

#include <cstddef>
#include <string>
#include <vector>

// Stores the current state of the player's weapon animation.

// Since Arena's weapon animations mostly share the same ordering, they can be hardcoded.
// Fists are an exception because they have fewer frames.

// The bow should not be used here because there is no bow animation in the Arena files;
// just a single idle frame that disappears when firing an arrow.

class ExeData;

class WeaponAnimation
{
public:
	enum class State
	{
		Sheathed, // Not displayed on-screen.
		Unsheathing,
		Idle,
		Forward,
		Down,
		Right,
		Left,
		DownRight,
		DownLeft,
		Firing, // Reserved for ranged weapons.
		Sheathing // Reverse of unsheathing animation.
	};
private:
	// Default time spent per animation frame.
	static const double DEFAULT_TIME_PER_FRAME;
	static const int FISTS_ID;

	WeaponAnimation::State state;
	int weaponID;
	std::string animationFilename;
	double currentTime;
	size_t rangeIndex;

	// Gets the time in seconds for each animation frame in the current state.
	double getTimePerFrame() const;

	// Gets the range of indices associated with the current animation state.
	const std::vector<int> &getCurrentRange() const;
public:
	WeaponAnimation(int weaponID, const ExeData &exeData);

	// Returns whether the animation is for a ranged weapon.
	bool isRanged() const;

	// Returns whether the weapon is currently sheathed (meaning it is not displayed).
	bool isSheathed() const;

	// Returns whether the weapon is currently not moving. This is relevant when
	// determining if the state can safely be changed without interrupting something.
	bool isIdle() const;

	// Gets the filename associated with the weapon (i.e., AXE, HAMMER, etc.).
	// This is used with the current index to determine which frame is drawn.
	const std::string &getAnimationFilename() const;

	// Gets the index into the .CIF animation's current frame. Do not call this
	// method if the sheathed animation is active.
	int getFrameIndex() const;

	// Sets the current weapon state. This resets the current animation index to the
	// beginning of the new state's range.
	void setState(WeaponAnimation::State state);

	// Ticks the weapon animation by delta time. If the weapon animation is swinging or
	// unsheathing, it will return to the idle animation automatically. If sheathing, it 
	// will return to the sheathed state automatically.
	void tick(double dt);
};

#endif
