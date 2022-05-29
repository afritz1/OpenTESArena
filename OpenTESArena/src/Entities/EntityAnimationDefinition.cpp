#include <cstdio>
#include <cstring>

#include "EntityAnimationDefinition.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

EntityAnimationDefinition::Keyframe::Keyframe(TextureAsset &&textureAsset, double width, double height)
	: textureAsset(std::move(textureAsset))
{
	this->width = width;
	this->height = height;
}

const TextureAsset &EntityAnimationDefinition::Keyframe::getTextureAsset() const
{
	return this->textureAsset;
}

double EntityAnimationDefinition::Keyframe::getWidth() const
{
	return this->width;
}

double EntityAnimationDefinition::Keyframe::getHeight() const
{
	return this->height;
}

EntityAnimationDefinition::KeyframeList::KeyframeList()
{
	this->flipped = false;
}

void EntityAnimationDefinition::KeyframeList::init(bool flipped)
{
	this->flipped = flipped;
}

int EntityAnimationDefinition::KeyframeList::getKeyframeCount() const
{
	return static_cast<int>(this->keyframes.size());
}

const EntityAnimationDefinition::Keyframe &EntityAnimationDefinition::KeyframeList::getKeyframe(int index) const
{
	DebugAssertIndex(this->keyframes, index);
	return this->keyframes[index];
}

bool EntityAnimationDefinition::KeyframeList::isFlipped() const
{
	return this->flipped;
}

void EntityAnimationDefinition::KeyframeList::addKeyframe(Keyframe &&keyframe)
{
	this->keyframes.push_back(std::move(keyframe));
}

void EntityAnimationDefinition::KeyframeList::clearKeyframes()
{
	this->keyframes.clear();
}

EntityAnimationDefinition::State::State()
{
	this->name.fill('\0');
	this->totalSeconds = 0.0;
	this->loop = false;
}

void EntityAnimationDefinition::State::init(const char *name, double totalSeconds, bool loop)
{
	DebugAssertMsg(!String::isNullOrEmpty(name), "State must have a name.");
	std::snprintf(this->name.data(), this->name.size(), "%s", name);

	this->totalSeconds = totalSeconds;
	this->loop = loop;
}

const char *EntityAnimationDefinition::State::getName() const
{
	return this->name.data();
}

int EntityAnimationDefinition::State::getKeyframeListCount() const
{
	return static_cast<int>(this->keyframeLists.size());
}

const EntityAnimationDefinition::KeyframeList &EntityAnimationDefinition::State::getKeyframeList(int index) const
{
	DebugAssertIndex(this->keyframeLists, index);
	return this->keyframeLists[index];
}

double EntityAnimationDefinition::State::getTotalSeconds() const
{
	return this->totalSeconds;
}

bool EntityAnimationDefinition::State::isLooping() const
{
	return this->loop;
}

void EntityAnimationDefinition::State::addKeyframeList(KeyframeList &&keyframeList)
{
	this->keyframeLists.push_back(std::move(keyframeList));
}

void EntityAnimationDefinition::State::clearKeyframeLists()
{
	this->keyframeLists.clear();
}

int EntityAnimationDefinition::getStateCount() const
{
	return static_cast<int>(this->states.size());
}

const EntityAnimationDefinition::State &EntityAnimationDefinition::getState(int index) const
{
	DebugAssertIndex(this->states, index);
	return this->states[index];
}

std::optional<int> EntityAnimationDefinition::tryGetStateIndex(const char *name) const
{
	if (String::isNullOrEmpty(name))
	{
		return std::nullopt;
	}

	const int stateCount = static_cast<int>(this->states.size());
	for (int i = 0; i < stateCount; i++)
	{
		const State &state = this->states[i];
		if (StringView::caseInsensitiveEquals(state.getName(), name))
		{
			return i;
		}
	}

	return std::nullopt;
}

void EntityAnimationDefinition::addState(State &&state)
{
	this->states.push_back(std::move(state));
}

void EntityAnimationDefinition::removeState(const char *name)
{
	const std::optional<int> stateIndex = this->tryGetStateIndex(name);
	if (stateIndex.has_value())
	{
		this->states.erase(this->states.begin() + *stateIndex);
	}
}

void EntityAnimationDefinition::clear()
{
	this->states.clear();
}
