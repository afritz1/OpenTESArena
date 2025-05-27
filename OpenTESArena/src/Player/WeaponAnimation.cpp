#include <algorithm>
#include <cmath>
#include <cstring>
#include <string_view>

#include "WeaponAnimation.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

WeaponAnimationDefinition::WeaponAnimationDefinition()
{
	this->stateCount = 0;
	this->frameCount = 0;
}

bool WeaponAnimationDefinition::tryGetStateIndex(const char *name, int *outIndex) const
{
	if (String::isNullOrEmpty(name))
	{
		return false;
	}

	const std::string_view nameView(name);
	for (int i = 0; i < this->stateCount; i++)
	{
		const WeaponAnimationDefinitionState &state = this->states[i];
		if (StringView::caseInsensitiveEquals(state.name, nameView))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

int WeaponAnimationDefinition::addState(const char *name, double seconds)
{
	if (String::isNullOrEmpty(name))
	{
		DebugLogError("Can't add animation state with no name.");
		return -1;
	}

	if (seconds < 0.0)
	{
		DebugLogError("Can't add animation state with negative period.");
		return -1;
	}

	if (this->stateCount == MAX_STATES)
	{
		DebugLogError("Can't add any more animation states.");
		return -1;
	}

	WeaponAnimationDefinitionState &state = this->states[this->stateCount];
	std::snprintf(state.name, std::size(state.name), "%s", name);
	state.seconds = seconds;
	state.framesIndex = this->frameCount;
	state.frameCount = 0;

	const int stateIndex = this->stateCount;
	this->stateCount++;
	return stateIndex;
}

int WeaponAnimationDefinition::addFrame(int stateIndex, const TextureAsset &textureAsset, int width, int height, int xOffset, int yOffset)
{
	if ((stateIndex < 0) || (stateIndex >= this->stateCount))
	{
		DebugLogError("Invalid state index " + std::to_string(stateIndex) + ".");
		return -1;
	}

	if (this->frameCount == MAX_FRAMES)
	{
		DebugLogError("Can't add any more animation frames.");
		return -1;
	}

	WeaponAnimationDefinitionState &state = this->states[stateIndex];
	state.frameCount++;

	WeaponAnimationDefinitionFrame &frame = this->frames[this->frameCount];
	frame.textureAsset = textureAsset;
	frame.width = width;
	frame.height = height;
	frame.xOffset = xOffset;
	frame.yOffset = yOffset;

	const int frameIndex = this->frameCount;
	this->frameCount++;
	return frameIndex;
}

WeaponAnimationInstance::WeaponAnimationInstance()
{
	this->clear();
}

bool WeaponAnimationInstance::isLooping() const
{
	return this->nextStateIndex == -1;
}

void WeaponAnimationInstance::addState(double targetSeconds)
{
	if (this->stateCount >= MAX_STATES)
	{
		DebugLogError("Can't add any more states.");
		return;
	}

	this->targetSecondsList[this->stateCount] = targetSeconds;
	this->stateCount++;
}

void WeaponAnimationInstance::setStateIndex(int index)
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->stateCount);
	this->currentSeconds = 0.0;
	this->targetSeconds = this->targetSecondsList[index];
	this->progressPercent = 0.0;
	this->currentStateIndex = index;
}

void WeaponAnimationInstance::setNextStateIndex(int index)
{
	this->nextStateIndex = index;
}

void WeaponAnimationInstance::resetTime()
{
	this->currentSeconds = 0.0;
	this->progressPercent = 0.0;
}

void WeaponAnimationInstance::clear()
{
	this->currentSeconds = 0.0;
	this->targetSeconds = 0.0;
	this->progressPercent = 0.0;
	this->currentStateIndex = -1;
	this->nextStateIndex = -1;
	this->stateCount = 0;
}

void WeaponAnimationInstance::update(double dt)
{
	this->currentSeconds += dt;
	this->progressPercent = std::clamp(this->currentSeconds / this->targetSeconds, 0.0, 1.0);

	if (this->progressPercent == 1.0)
	{
		if (this->isLooping())
		{
			this->currentSeconds = std::fmod(this->currentSeconds, this->targetSeconds);
		}
		else
		{
			this->setStateIndex(this->nextStateIndex);
			this->setNextStateIndex(-1);
		}
	}
}
