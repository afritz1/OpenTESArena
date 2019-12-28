#include <algorithm>
#include <cmath>

#include "EntityAnimationData.h"

#include "components/debug/Debug.h"

EntityAnimationData::Keyframe::Keyframe(double width, double height, int textureID)
{
	DebugAssert(width >= 0.0);
	DebugAssert(height >= 0.0);
	DebugAssert(textureID >= 0);

	this->width = width;
	this->height = height;
	this->textureID = textureID;
}

double EntityAnimationData::Keyframe::getWidth() const
{
	return this->width;
}

double EntityAnimationData::Keyframe::getHeight() const
{
	return this->height;
}

int EntityAnimationData::Keyframe::getTextureID() const
{
	return this->textureID;
}

EntityAnimationData::State::State(StateType type, double secondsPerFrame, bool loop, bool flipped)
{
	DebugAssert(secondsPerFrame > 0.0);

	this->type = type;
	this->secondsPerFrame = secondsPerFrame;
	this->loop = loop;
	this->flipped = flipped;
}

EntityAnimationData::StateType EntityAnimationData::State::getType() const
{
	return this->type;
}

bool EntityAnimationData::State::isFlipped() const
{
	return this->flipped;
}

bool EntityAnimationData::State::getLoop() const
{
	return this->loop;
}

double EntityAnimationData::State::getSecondsPerFrame() const
{
	return this->secondsPerFrame;
}

const std::string &EntityAnimationData::State::getTextureName() const
{
	return this->textureName;
}

BufferView<const EntityAnimationData::Keyframe> EntityAnimationData::State::getKeyframes() const
{
	return BufferView(this->keyframes.data(), static_cast<int>(this->keyframes.size()));
}

void EntityAnimationData::State::setTextureName(std::string &&textureName)
{
	this->textureName = std::move(textureName);
}

void EntityAnimationData::State::addKeyframe(Keyframe &&keyframe)
{
	this->keyframes.push_back(std::move(keyframe));
}

void EntityAnimationData::State::clearKeyframes()
{
	this->keyframes.clear();
}

EntityAnimationData::Instance::Instance()
{
	this->stateType = StateType::Idle;
	this->percentDone = 0.0;
}

const std::vector<EntityAnimationData::State> &EntityAnimationData::Instance::getStateList(
	const EntityAnimationData &animationData) const
{
	const std::vector<State> *stateList = animationData.findStateList(this->stateType);
	DebugAssertMsg(stateList != nullptr, "Couldn't find state list \"" +
		std::to_string(static_cast<int>(this->stateType)) + "\".");
	return *stateList;
}

int EntityAnimationData::Instance::getKeyframeIndex(int stateIndex,
	const EntityAnimationData &animationData) const
{
	const std::vector<State> &stateList = this->getStateList(animationData);

	DebugAssertIndex(stateList, stateIndex);
	const State &state = stateList[stateIndex];
	const BufferView<const Keyframe> keyframes = state.getKeyframes();
	const int keyframeCount = keyframes.getCount();

	// Can't have an animation state with no keyframes.
	DebugAssert(keyframeCount > 0);

	const double realIndex = this->percentDone * static_cast<double>(keyframeCount);
	const int index = std::clamp(static_cast<int>(realIndex), 0, keyframeCount - 1);
	return index;
}

void EntityAnimationData::Instance::setStateType(StateType stateType)
{
	this->stateType = stateType;
	this->resetTime();
}

void EntityAnimationData::Instance::resetTime()
{
	this->percentDone = 0.0;
}

void EntityAnimationData::Instance::reset()
{
	this->stateType = StateType::Idle;
	this->resetTime();
}

void EntityAnimationData::Instance::tick(double dt, const EntityAnimationData &animationData)
{
	const std::vector<State> &stateList = this->getStateList(animationData);

	// @todo: see if total seconds (i.e. period) instead of seconds per frame would be better.
	// - Don't know if we can assume that all state lists are the same size (probably not).
	DebugAssert(stateList.size() > 0);
	const State &state = stateList[0];

	const BufferView<const Keyframe> keyframes = state.getKeyframes();
	const int keyframeCount = keyframes.getCount();
	const double secondsPerFrame = state.getSecondsPerFrame();

	const double targetSeconds = keyframeCount * secondsPerFrame;
	const double currentSeconds = (this->percentDone * targetSeconds) + dt;

	// Calculate percent done based on loop behavior.
	this->percentDone = state.getLoop() ?
		std::fmod(currentSeconds / targetSeconds, 1.0) :
		std::clamp(currentSeconds / targetSeconds, 0.0, 1.0);
}

const std::vector<EntityAnimationData::State> *EntityAnimationData::findStateList(
	StateType stateType) const
{
	const auto iter = std::find_if(this->stateLists.begin(), this->stateLists.end(),
		[stateType](const std::vector<EntityAnimationData::State> &stateList)
	{
		DebugAssert(stateList.size() > 0);
		const EntityAnimationData::State &state = stateList[0];
		return state.getType() == stateType;
	});

	return (iter != this->stateLists.end()) ? &(*iter) : nullptr;
}

bool EntityAnimationData::hasStateList(StateType stateType) const
{
	return this->findStateList(stateType) != nullptr;
}

void EntityAnimationData::addStateList(std::vector<State> &&stateList)
{
	DebugAssert(stateList.size() > 0);

	// Can't have two state lists of the same type.
	DebugAssert(this->findStateList(stateList[0].getType()) == nullptr);

	this->stateLists.push_back(std::move(stateList));
}

void EntityAnimationData::removeStateList(StateType stateType)
{
	const std::vector<State> *stateList = this->findStateList(stateType);

	if (stateList != nullptr)
	{
		DebugAssert(stateList->size() > 0);

		const std::vector<State> *firstStateList = this->stateLists.data();
		const int index = static_cast<int>(std::distance(firstStateList, stateList));
		this->stateLists.erase(this->stateLists.begin() + index);
	}
}

void EntityAnimationData::clear()
{
	this->stateLists.clear();
}
