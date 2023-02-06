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

EntityAnimationInstance::EntityAnimationInstance(const EntityAnimationInstance &other)
	: states(other.states)
{
	const std::unique_ptr<CitizenParams> &otherCitizenParams = other.citizenParams;
	this->citizenParams = (otherCitizenParams != nullptr) ?
		std::make_unique<CitizenParams>(*otherCitizenParams) : nullptr;

	this->currentSeconds = other.currentSeconds;
	this->stateIndex = other.stateIndex;
}

EntityAnimationInstance &EntityAnimationInstance::operator=(const EntityAnimationInstance &other)
{
	if (this != &other)
	{
		this->states = other.states;

		const std::unique_ptr<CitizenParams> &otherCitizenParams = other.citizenParams;
		this->citizenParams = (otherCitizenParams != nullptr) ?
			std::make_unique<CitizenParams>(*otherCitizenParams) : nullptr;

		this->currentSeconds = other.currentSeconds;
		this->stateIndex = other.stateIndex;
	}
	
	return *this;
}

void EntityAnimationInstance::init(const EntityAnimationDefinition &animDef)
{
	for (int i = 0; i < animDef.getStateCount(); i++)
	{
		const EntityAnimationDefinition::State &defState = animDef.getState(i);
		EntityAnimationInstance::State instState;

		for (int j = 0; j < defState.getKeyframeListCount(); j++)
		{
			const EntityAnimationDefinition::KeyframeList &defKeyframeList = defState.getKeyframeList(j);
			EntityAnimationInstance::KeyframeList instKeyframeList;

			for (int k = 0; k < defKeyframeList.getKeyframeCount(); k++)
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

const EntityAnimationInstance::CitizenParams *EntityAnimationInstance::getCitizenParams() const
{
	return this->citizenParams.get();
}

int EntityAnimationInstance::getStateIndex() const
{
	return this->stateIndex;
}

double EntityAnimationInstance::getCurrentSeconds() const
{
	return this->currentSeconds;
}

void EntityAnimationInstance::setCitizenParams(std::unique_ptr<CitizenParams> &&citizenParams)
{
	this->citizenParams = std::move(citizenParams);
}

void EntityAnimationInstance::setStateIndex(int index)
{
	this->stateIndex = index;
	this->resetTime();
}

void EntityAnimationInstance::reset()
{
	this->states.clear();
	this->citizenParams = nullptr;
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

EntityAnimationInstanceKeyframeList::EntityAnimationInstanceKeyframeList()
{
	this->textureIdsIndex = -1;
	this->textureIdCount = 0;
}

EntityAnimationInstanceState::EntityAnimationInstanceState()
{
	this->keyframeListsIndex = -1;
	this->keyframeListCount = 0;
	this->seconds = 0.0;
	this->looping = false;
}

EntityAnimationInstanceA::EntityAnimationInstanceA()
{
	this->clear();
}

void EntityAnimationInstanceA::init(const EntityAnimationDefinition &def, const BufferView<const ScopedObjectTextureRef> &textureRefs)
{
	this->currentSeconds = 0.0;

	const int defStateCount = def.getStateCount();
	this->stateCount = defStateCount;
	this->keyframeListCount = 0;
	this->textureIdCount = 0;

	int textureWriteIndex = 0;
	for (int i = 0; i < defStateCount; i++)
	{
		const EntityAnimationDefinition::State &defState = def.getState(i);
		const int defKeyframeListCount = defState.getKeyframeListCount();

		DebugAssertIndex(this->states, i);
		EntityAnimationInstanceState &instState = this->states[i];
		instState.keyframeListsIndex = this->keyframeListCount;
		instState.keyframeListCount = defKeyframeListCount;
		instState.seconds = defState.getTotalSeconds();
		instState.looping = defState.isLooping();

		for (int j = 0; j < defKeyframeListCount; j++)
		{
			const EntityAnimationDefinition::KeyframeList &defKeyframeList = defState.getKeyframeList(j);
			const int defKeyframeCount = defKeyframeList.getKeyframeCount();

			const int instKeyframeListIndex = this->keyframeListCount + j;
			DebugAssertIndex(this->keyframeLists, instKeyframeListIndex);
			EntityAnimationInstanceKeyframeList &instKeyframeList = this->keyframeLists[instKeyframeListIndex];
			instKeyframeList.textureIdsIndex = this->textureIdCount;
			instKeyframeList.textureIdCount = defKeyframeCount;

			for (int k = 0; k < defKeyframeCount; k++)
			{
				const int textureIdIndex = this->textureIdCount + k;
				const ScopedObjectTextureRef &textureRef = textureRefs.get(textureWriteIndex);

				DebugAssertIndex(this->textureIDs, textureIdIndex);
				ObjectTextureID &textureID = this->textureIDs[textureIdIndex];
				textureID = textureRef.get();
				textureWriteIndex++;
			}

			this->textureIdCount += defKeyframeCount;
		}

		this->keyframeListCount += defKeyframeListCount;
	}

	// This function doesn't set the initial state index; the caller is expected to.
	DebugAssert(this->currentStateIndex == -1);
}

void EntityAnimationInstanceA::setStateIndex(int index)
{
	DebugAssert(this->stateCount > 0);
	DebugAssert(index >= 0);
	DebugAssert(index < this->stateCount);
	const EntityAnimationInstanceState &currentState = this->states[index];

	// Start at the beginning of this state.
	this->currentStateIndex = index;
	this->currentSeconds = 0.0;
	this->targetSeconds = currentState.seconds;
	this->looping = currentState.looping;

	const int currentKeyframeListsIndex = currentState.keyframeListsIndex;
	DebugAssert(currentState.keyframeListCount > 0);
	DebugAssert(currentKeyframeListsIndex >= 0);
	DebugAssert(currentKeyframeListsIndex < this->keyframeListCount);
	const EntityAnimationInstanceKeyframeList &currentKeyframeList = this->keyframeLists[currentKeyframeListsIndex];
	
	const int currentTextureIdIndex = currentKeyframeList.textureIdsIndex;
	DebugAssert(currentKeyframeList.textureIdCount > 0);
	DebugAssert(currentTextureIdIndex >= 0);
	DebugAssert(currentTextureIdIndex < this->textureIdCount);
	this->currentTextureID = this->textureIDs[currentTextureIdIndex];
}

void EntityAnimationInstanceA::resetTime()
{
	this->currentSeconds = 0.0;
}

void EntityAnimationInstanceA::clear()
{
	std::fill(std::begin(this->states), std::end(this->states), EntityAnimationInstanceState());
	this->stateCount = 0;

	std::fill(std::begin(this->keyframeLists), std::end(this->keyframeLists), EntityAnimationInstanceKeyframeList());
	this->keyframeListCount = 0;

	std::fill(std::begin(this->textureIDs), std::end(this->textureIDs), -1);
	this->textureIdCount = 0;

	this->currentSeconds = 0.0;
	this->targetSeconds = 0.0;
	this->currentStateIndex = -1;
	this->currentTextureID = -1;
	this->looping = false;
}

void EntityAnimationInstanceA::update(double dt)
{
	if (this->looping)
	{
		this->currentSeconds = std::fmod(this->currentSeconds + dt, this->targetSeconds);
	}
	else
	{
		this->currentSeconds = std::min(this->currentSeconds + dt, this->targetSeconds);
	}
}
