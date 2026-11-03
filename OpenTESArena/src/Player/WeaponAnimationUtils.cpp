#include <cmath>

#include "WeaponAnimation.h"
#include "WeaponAnimationUtils.h"
#include "../Math/MathUtils.h"

#include "components/utilities/String.h"

bool WeaponAnimationUtils::isSheathed(const WeaponAnimationDefinitionState &state)
{
	return StringView::equals(state.name, WeaponAnimationUtils::STATE_SHEATHED);
}

bool WeaponAnimationUtils::isUnsheathing(const WeaponAnimationDefinitionState &state)
{
	return StringView::equals(state.name, WeaponAnimationUtils::STATE_UNSHEATHING);
}

bool WeaponAnimationUtils::isSheathing(const WeaponAnimationDefinitionState &state)
{
	return StringView::equals(state.name, WeaponAnimationUtils::STATE_SHEATHING);
}

bool WeaponAnimationUtils::isIdle(const WeaponAnimationDefinitionState &state)
{
	return StringView::equals(state.name, WeaponAnimationUtils::STATE_IDLE);
}

int WeaponAnimationUtils::getFrameIndex(const WeaponAnimationInstance &animInst, const WeaponAnimationDefinition &animDef)
{
	DebugAssertIndex(animDef.states, animInst.currentStateIndex);
	const WeaponAnimationDefinitionState &state = animDef.states[animInst.currentStateIndex];
	const int interpolatedIndex = MathUtils::getInterpolatedIndex(state.frameCount, animInst.progressPercent);
	return state.framesIndex + interpolatedIndex;
}
