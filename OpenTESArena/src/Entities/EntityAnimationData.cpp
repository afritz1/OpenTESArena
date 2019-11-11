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

EntityAnimationData::State::State(StateType type, double secondsPerFrame, bool loop)
{
	DebugAssert(secondsPerFrame > 0.0);

	this->type = type;
	this->secondsPerFrame = secondsPerFrame;
	this->loop = loop;
}

EntityAnimationData::StateType EntityAnimationData::State::getType() const
{
	return this->type;
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

const EntityAnimationData::State &EntityAnimationData::Instance::getState(
	const EntityAnimationData &animationData) const
{
	const State *state = animationData.findState(this->stateType);
	DebugAssertMsg(state != nullptr, "Couldn't find state \"" +
		std::to_string(static_cast<int>(this->stateType)) + "\".");
	return *state;
}

int EntityAnimationData::Instance::getKeyframeIndex(const EntityAnimationData &animationData) const
{
	const State &state = this->getState(animationData);
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
	const State &state = this->getState(animationData);
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

const EntityAnimationData::State *EntityAnimationData::findState(StateType stateType) const
{
	const auto iter = std::find_if(this->states.begin(), this->states.end(),
		[stateType](const EntityAnimationData::State &state)
	{
		return state.getType() == stateType;
	});

	return (iter != this->states.end()) ? &(*iter) : nullptr;
}

void EntityAnimationData::addState(State &&state)
{
	// Can't have two states of the same type.
	DebugAssert(this->findState(state.getType()) == nullptr);

	this->states.push_back(std::move(state));
}

void EntityAnimationData::removeState(StateType stateType)
{
	const State *state = this->findState(stateType);

	if (state != nullptr)
	{
		const State *firstState = this->states.data();
		const int index = static_cast<int>(std::distance(firstState, state));
		this->states.erase(this->states.begin() + index);
	}
}

void EntityAnimationData::clear()
{
	this->states.clear();
}
