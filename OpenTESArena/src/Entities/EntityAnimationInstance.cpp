#include <algorithm>
#include <cmath>

#include "EntityAnimationInstance.h"
#include "../Assets/TextureManager.h"

#include "components/debug/Debug.h"

int EntityAnimationInstance::KeyframeList::getKeyframeCount() const
{
	return static_cast<int>(this->keyframes.size());
}

const EntityAnimationInstance::Keyframe &EntityAnimationInstance::KeyframeList::getKeyframe(int index) const
{
	DebugAssertIndex(this->keyframes, index);
	return this->keyframes[index];
}

void EntityAnimationInstance::KeyframeList::addKeyframe(Keyframe &&keyframe)
{
	this->keyframes.push_back(std::move(keyframe));
}

void EntityAnimationInstance::KeyframeList::clearKeyframes()
{
	this->keyframes.clear();
}

int EntityAnimationInstance::State::getKeyframeListCount() const
{
	return static_cast<int>(this->keyframeLists.size());
}

const EntityAnimationInstance::KeyframeList &EntityAnimationInstance::State::getKeyframeList(int index) const
{
	DebugAssertIndex(this->keyframeLists, index);
	return this->keyframeLists[index];
}

void EntityAnimationInstance::State::addKeyframeList(EntityAnimationInstance::KeyframeList &&keyframeList)
{
	this->keyframeLists.push_back(std::move(keyframeList));
}

void EntityAnimationInstance::State::clearKeyframeLists()
{
	this->keyframeLists.clear();
}

EntityAnimationInstance::EntityAnimationInstance()
{
	this->stateIndex = -1;
	this->currentSeconds = 0.0;
}

void EntityAnimationInstance::init(const EntityAnimationDefinition &animDef)
{
	for (int i = 0; i < animDef.stateCount; i++)
	{
		const EntityAnimationDefinitionState &defState = animDef.states[i];
		EntityAnimationInstance::State instState;

		for (int j = 0; j < defState.keyframeListCount; j++)
		{
			const int keyframeListsIndex = defState.keyframeListsIndex + j;
			const EntityAnimationDefinitionKeyframeList &defKeyframeList = animDef.keyframeLists[keyframeListsIndex];
			EntityAnimationInstance::KeyframeList instKeyframeList;

			for (int k = 0; k < defKeyframeList.keyframeCount; k++)
			{
				EntityAnimationInstance::Keyframe instKeyframe;
				// @todo: this will eventually get renderer entity texture handles, likely from looking up in a hash table
				// of texture asset references (w/ other variables like flipped, reflective, etc.).
				instKeyframeList.addKeyframe(std::move(instKeyframe));
			}

			instState.addKeyframeList(std::move(instKeyframeList));
		}

		this->states.emplace_back(std::move(instState));
	}
}

int EntityAnimationInstance::getStateCount() const
{
	return static_cast<int>(this->states.size());
}

const EntityAnimationInstance::State &EntityAnimationInstance::getState(int index) const
{
	DebugAssertIndex(this->states, index);
	return this->states[index];
}

int EntityAnimationInstance::getStateIndex() const
{
	return this->stateIndex;
}

double EntityAnimationInstance::getCurrentSeconds() const
{
	return this->currentSeconds;
}

void EntityAnimationInstance::setStateIndex(int index)
{
	this->stateIndex = index;
	this->resetTime();
}

void EntityAnimationInstance::reset()
{
	this->states.clear();
	this->resetTime();
	this->stateIndex = -1;
}

void EntityAnimationInstance::resetTime()
{
	this->currentSeconds = 0.0;
}

void EntityAnimationInstance::tick(double dt, double totalSeconds, bool looping)
{
	this->currentSeconds += dt;
	if (looping && (this->currentSeconds >= totalSeconds))
	{
		this->currentSeconds = std::fmod(this->currentSeconds, totalSeconds);
	}
}

EntityAnimationInstanceA::EntityAnimationInstanceA()
{
	this->clear();
}

void EntityAnimationInstanceA::addState(double targetSeconds, bool isLooping)
{
	if (this->stateCount >= MAX_STATES)
	{
		DebugLogError("Can't add any more states.");
		return;
	}

	this->targetSecondsList[this->stateCount] = targetSeconds;
	this->isLoopingList[this->stateCount] = isLooping;
	this->stateCount++;
}

void EntityAnimationInstanceA::setStateIndex(int index)
{
	DebugAssert(index >= 0);
	DebugAssert(index < this->stateCount);
	this->currentSeconds = 0.0;
	this->targetSeconds = this->targetSecondsList[index];
	this->progressPercent = 0.0;
	this->currentStateIndex = index;
	this->isLooping = this->isLoopingList[index];
}

void EntityAnimationInstanceA::resetTime()
{
	this->currentSeconds = 0.0;
	this->progressPercent = 0.0;
}

void EntityAnimationInstanceA::clear()
{
	this->currentSeconds = 0.0;
	this->targetSeconds = 0.0;
	this->progressPercent = 0.0;
	this->currentStateIndex = -1;
	this->stateCount = 0;
	this->isLooping = false;
}

void EntityAnimationInstanceA::update(double dt)
{
	if (this->isLooping)
	{
		this->currentSeconds = std::fmod(this->currentSeconds + dt, this->targetSeconds);
	}
	else
	{
		this->currentSeconds = std::min(this->currentSeconds + dt, this->targetSeconds);
	}

	this->progressPercent = std::clamp(this->currentSeconds / this->targetSeconds, 0.0, 1.0);
}
