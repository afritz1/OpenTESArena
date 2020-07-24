#include <cmath>

#include "EntityAnimationInstance.h"

EntityAnimationInstance::Keyframe::Keyframe(int textureID)
{
	this->textureID = textureID;
}

int EntityAnimationInstance::Keyframe::getTextureID() const
{
	return this->textureID;
}

BufferView<const EntityAnimationInstance::Keyframe> EntityAnimationInstance::State::getKeyframes() const
{
	return BufferView<const Keyframe>(this->keyframes.data(), static_cast<int>(this->keyframes.size()));
}

const std::string &EntityAnimationInstance::State::getTextureName() const
{
	return this->textureName;
}

void EntityAnimationInstance::State::addKeyframe(Keyframe &&keyframe)
{
	this->keyframes.push_back(std::move(keyframe));
}

void EntityAnimationInstance::State::clearKeyframes()
{
	this->keyframes.clear();
}

void EntityAnimationInstance::State::setTextureName(std::string &&textureName)
{
	this->textureName = std::move(textureName);
}

BufferView<const EntityAnimationInstance::State> EntityAnimationInstance::StateList::getStates() const
{
	return BufferView<const State>(this->states.data(), static_cast<int>(this->states.size()));
}

void EntityAnimationInstance::StateList::addState(EntityAnimationInstance::State &&state)
{
	this->states.push_back(std::move(state));
}

void EntityAnimationInstance::StateList::clearStates()
{
	this->states.clear();
}

EntityAnimationInstance::EntityAnimationInstance()
{
	this->currentSeconds = 0.0;
	this->stateListIndex = -1;
	this->animID = EntityAnimationLibrary::NO_ID;
}

int EntityAnimationInstance::getStateListCount() const
{
	return static_cast<int>(this->stateLists.size());
}

BufferView<const EntityAnimationInstance::StateList> EntityAnimationInstance::getStateLists() const
{
	return BufferView<const StateList>(this->stateLists.data(), static_cast<int>(this->stateLists.size()));
}

int EntityAnimationInstance::getStateListIndex() const
{
	return this->stateListIndex;
}

double EntityAnimationInstance::getCurrentSeconds() const
{
	return this->currentSeconds;
}

EntityAnimID EntityAnimationInstance::getAnimID() const
{
	return this->animID;
}

void EntityAnimationInstance::addStateList(StateList &&stateList)
{
	this->stateLists.push_back(std::move(stateList));
}

void EntityAnimationInstance::clearStateLists()
{
	this->stateLists.clear();
}

void EntityAnimationInstance::setStateListIndex(int index)
{
	this->stateListIndex = index;
	this->resetTime();
}

void EntityAnimationInstance::setAnimID(EntityAnimID animID)
{
	this->animID = animID;
}

void EntityAnimationInstance::resetTime()
{
	this->currentSeconds = 0.0;
}

void EntityAnimationInstance::tick(double dt, const EntityAnimationDefinition::State &defState)
{
	this->currentSeconds += dt;

	const double totalSeconds = defState.getTotalSeconds();
	if (defState.isLooping() && (this->currentSeconds >= totalSeconds))
	{
		this->currentSeconds = std::fmod(this->currentSeconds, totalSeconds);
	}
}
