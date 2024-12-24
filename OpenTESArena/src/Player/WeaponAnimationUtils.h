#ifndef WEAPON_ANIMATION_UTILS_H
#define WEAPON_ANIMATION_UTILS_H

#include <string>

struct WeaponAnimationDefinition;
struct WeaponAnimationDefinitionState;
struct WeaponAnimationInstance;

namespace WeaponAnimationUtils
{
	const std::string STATE_SHEATHED = "Sheathed";
	const std::string STATE_UNSHEATHING = "Unsheathing";
	const std::string STATE_SHEATHING = "Sheathing";
	const std::string STATE_IDLE = "Idle";
	const std::string STATE_FORWARD = "Forward";
	const std::string STATE_DOWN = "Down";
	const std::string STATE_RIGHT = "Right";
	const std::string STATE_LEFT = "Left";
	const std::string STATE_DOWN_RIGHT = "DownRight";
	const std::string STATE_DOWN_LEFT = "DownLeft";
	const std::string STATE_FIRING = "Firing";

	constexpr int MAX_NAME_LENGTH = 32;

	bool isSheathed(const WeaponAnimationDefinitionState &state);
	bool isUnsheathing(const WeaponAnimationDefinitionState &state);
	bool isSheathing(const WeaponAnimationDefinitionState &state);
	bool isIdle(const WeaponAnimationDefinitionState &state);

	int getFrameIndex(const WeaponAnimationInstance &animInst, const WeaponAnimationDefinition &animDef);
}

#endif
