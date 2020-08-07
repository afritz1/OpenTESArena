#ifndef ENTITY_ANIMATION_UTILS_H
#define ENTITY_ANIMATION_UTILS_H

#include <string>

namespace EntityAnimationUtils
{
	const std::string STATE_IDLE = "Idle";
	const std::string STATE_LOOK = "Look";
	const std::string STATE_WALK = "Walk";
	const std::string STATE_ATTACK = "Attack";
	const std::string STATE_DEATH = "Death";
	const std::string STATE_ACTIVATED = "Activated";

	// Max length of animation state name.
	constexpr int NAME_LENGTH = 32;
}

#endif
