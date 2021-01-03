#include <cmath>

#include "EntityAnimationInstance.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureInstanceManager.h"

#include "components/debug/Debug.h"

EntityAnimationInstance::Keyframe EntityAnimationInstance::Keyframe::makeFromTextureBuilderID(
	TextureBuilderID overrideTextureBuilderID)
{
	Keyframe keyframe;
	keyframe.overrideTextureBuilderID = overrideTextureBuilderID;
	return keyframe;
}

const TextureBuilder &EntityAnimationInstance::Keyframe::getTextureBuilderHandle(
	const EntityAnimationDefinition::Keyframe &defKeyframe, TextureManager &textureManager) const
{
	TextureBuilderID textureBuilderID;
	if (this->overrideTextureBuilderID.has_value())
	{
		textureBuilderID = *this->overrideTextureBuilderID;
	}
	else
	{
		const TextureAssetReference &textureAssetRef = defKeyframe.getTextureAssetRef();
		const std::string &filename = textureAssetRef.filename;
		const std::optional<TextureBuilderIdGroup> ids = textureManager.tryGetTextureBuilderIDs(filename.c_str());
		if (!ids.has_value())
		{
			DebugCrash("Couldn't get texture builder IDs for \"" + filename + "\".");
		}

		const int textureIndex = textureAssetRef.index.has_value() ? *textureAssetRef.index : 0;
		textureBuilderID = ids->getID(textureIndex);
	}

	return textureManager.getTextureBuilderHandle(textureBuilderID);
}

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

void EntityAnimationInstance::addState(State &&state)
{
	this->states.push_back(std::move(state));
}

void EntityAnimationInstance::clearStates()
{
	this->states.clear();
}

void EntityAnimationInstance::setStateIndex(int index)
{
	this->stateIndex = index;
	this->resetTime();
}

void EntityAnimationInstance::reset()
{
	this->stateIndex = -1;
	this->resetTime();
	this->states.clear();
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
