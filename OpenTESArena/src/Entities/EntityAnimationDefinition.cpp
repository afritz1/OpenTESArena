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

bool EntityAnimationDefinition::Keyframe::operator==(const Keyframe &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->textureAsset != other.textureAsset)
	{
		return false;
	}

	if (this->width != other.width)
	{
		return false;
	}

	if (this->height != other.height)
	{
		return false;
	}

	return true;
}

bool EntityAnimationDefinition::Keyframe::operator!=(const Keyframe &other) const
{
	return !(*this == other);
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

bool EntityAnimationDefinition::KeyframeList::operator==(const KeyframeList &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->keyframes.size() != other.keyframes.size())
	{
		return false;
	}

	for (size_t i = 0; i < this->keyframes.size(); i++)
	{
		const EntityAnimationDefinition::Keyframe &keyframe = this->keyframes[i];
		const EntityAnimationDefinition::Keyframe &otherKeyframe = other.keyframes[i];
		if (keyframe != otherKeyframe)
		{
			return false;
		}
	}

	if (this->flipped != other.flipped)
	{
		return false;
	}

	return true;
}

bool EntityAnimationDefinition::KeyframeList::operator!=(const KeyframeList &other) const
{
	return !(*this == other);
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

bool EntityAnimationDefinition::State::operator==(const State &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (std::strncmp(this->name.data(), other.name.data(), this->name.size()) != 0)
	{
		return false;
	}

	if (this->keyframeLists.size() != other.keyframeLists.size())
	{
		return false;
	}

	for (size_t i = 0; i < this->keyframeLists.size(); i++)
	{
		const EntityAnimationDefinition::KeyframeList &keyframeList = this->keyframeLists[i];
		const EntityAnimationDefinition::KeyframeList &otherKeyframeList = other.keyframeLists[i];
		if (keyframeList != otherKeyframeList)
		{
			return false;
		}
	}

	if (this->totalSeconds != other.totalSeconds)
	{
		return false;
	}

	if (this->loop != other.loop)
	{
		return false;
	}

	return true;
}

bool EntityAnimationDefinition::State::operator!=(const State &other) const
{
	return !(*this == other);
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

bool EntityAnimationDefinition::operator==(const EntityAnimationDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->states.size() != other.states.size())
	{
		return false;
	}

	for (size_t i = 0; i < this->states.size(); i++)
	{
		const EntityAnimationDefinition::State &state = this->states[i];
		const EntityAnimationDefinition::State &otherState = other.states[i];
		if (state != otherState)
		{
			return false;
		}
	}

	return true;
}

bool EntityAnimationDefinition::operator!=(const EntityAnimationDefinition &other) const
{
	return !(*this == other);
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

int EntityAnimationDefinition::getTotalKeyframeCount() const
{
	int count = 0;
	for (int i = 0; i < this->getStateCount(); i++)
	{
		const State &state = this->states[i];
		for (int j = 0; j < state.getKeyframeListCount(); j++)
		{
			const KeyframeList &keyframeList = state.getKeyframeList(j);
			count += keyframeList.getKeyframeCount();
		}
	}

	return count;
}

int EntityAnimationDefinition::getLinearizedKeyframeIndex(int stateIndex, int keyframeListIndex, int keyframeIndex) const
{
	DebugAssert(stateIndex >= 0);
	DebugAssert(keyframeListIndex >= 0);
	DebugAssert(keyframeIndex >= 0);

	int index = 0;
	for (int i = 0; i < this->getStateCount(); i++)
	{
		const State &state = this->states[i];
		if (i < stateIndex)
		{
			for (int j = 0; j < state.getKeyframeListCount(); j++)
			{
				const KeyframeList &keyframeList = state.getKeyframeList(j);
				index += keyframeList.getKeyframeCount();
			}
		}
		else if (i == stateIndex)
		{
			for (int j = 0; j < state.getKeyframeListCount(); j++)
			{
				const KeyframeList &keyframeList = state.getKeyframeList(j);
				if (j < keyframeListIndex)
				{
					index += keyframeList.getKeyframeCount();
				}
				else if (j == keyframeListIndex)
				{
					for (int k = 0; i < keyframeList.getKeyframeCount(); k++)
					{
						if (k <= keyframeIndex)
						{
							index++;
						}
					}
				}
			}
		}
	}

	return index;
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

EntityAnimationDefinitionA::EntityAnimationDefinitionA()
{
	this->stateCount = 0;
	this->keyframeListCount = 0;
	this->keyframeCount = 0;
}

std::optional<int> EntityAnimationDefinitionA::tryGetStateIndex(const char *name) const
{
	if (String::isNullOrEmpty(name))
	{
		return std::nullopt;
	}

	const std::string_view nameView(name);
	for (int i = 0; i < this->stateCount; i++)
	{
		const EntityAnimationDefinitionState &state = this->states[i];
		if (StringView::caseInsensitiveEquals(state.name, nameView))
		{
			return i;
		}
	}

	return std::nullopt;
}

int EntityAnimationDefinitionA::getLinearizedKeyframeIndex(int stateIndex, int keyframeListIndex, int keyframeIndex) const
{
	DebugAssert(stateIndex >= 0);
	DebugAssert(keyframeListIndex >= 0);
	DebugAssert(keyframeIndex >= 0);

	int index = 0;
	for (int i = 0; i < this->stateCount; i++)
	{
		const EntityAnimationDefinitionState &state = this->states[i];
		if (i < stateIndex)
		{
			for (int j = 0; j < state.keyframeListCount; j++)
			{
				const int curKeyframeListIndex = state.keyframeListsIndex + j;
				DebugAssert(curKeyframeListIndex < this->keyframeListCount);
				const EntityAnimationDefinitionKeyframeList &keyframeList = this->keyframeLists[curKeyframeListIndex];
				index += keyframeList.keyframeCount;
			}
		}
		else if (i == stateIndex)
		{
			for (int j = 0; j < state.keyframeListCount; j++)
			{
				const int curKeyframeListIndex = state.keyframeListsIndex + j;
				DebugAssert(curKeyframeListIndex < this->keyframeListCount);
				const EntityAnimationDefinitionKeyframeList &keyframeList = this->keyframeLists[curKeyframeListIndex];
				if (j < keyframeListIndex)
				{
					index += keyframeList.keyframeCount;
				}
				else if (j == keyframeListIndex)
				{
					for (int k = 0; i < keyframeList.keyframeCount; k++)
					{
						if (k <= keyframeIndex)
						{
							index++;
						}
					}
				}
			}
		}
	}

	return index;
}

int EntityAnimationDefinitionA::addState(const char *name, double seconds, bool isLooping)
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
		DebugLogError("Can't add anymore animation states.");
		return -1;
	}

	EntityAnimationDefinitionState &state = this->states[this->stateCount];
	std::snprintf(state.name, std::size(state.name), "%s", name);
	state.seconds = seconds;
	state.keyframeListsIndex = this->keyframeListCount;
	state.keyframeListCount = 0;
	state.isLooping = isLooping;

	const int stateIndex = this->stateCount;
	this->stateCount++;
	return stateIndex;
}

int EntityAnimationDefinitionA::addKeyframeList(int stateIndex, bool flipped)
{
	if ((stateIndex < 0) || (stateIndex > this->stateCount))
	{
		DebugLogError("Invalid state index " + std::to_string(stateIndex) + ".");
		return -1;
	}

	if (this->keyframeListCount == MAX_KEYFRAME_LISTS)
	{
		DebugLogError("Can't add anymore animation keyframe lists.");
		return -1;
	}

	EntityAnimationDefinitionState &state = this->states[this->stateCount];
	state.keyframeListCount++;

	EntityAnimationDefinitionKeyframeList &keyframeList = this->keyframeLists[this->keyframeListCount];
	keyframeList.keyframesIndex = this->keyframeCount;
	keyframeList.keyframeCount = 0;
	keyframeList.flipped = flipped;

	const int keyframeListIndex = this->keyframeListCount;
	this->keyframeListCount++;
	return keyframeListIndex;
}

int EntityAnimationDefinitionA::addKeyframe(int keyframeListIndex, TextureAsset &&textureAsset, double width, double height)
{
	if ((keyframeListIndex < 0) || (keyframeListIndex > this->keyframeListCount))
	{
		DebugLogError("Invalid keyframe list index " + std::to_string(keyframeListIndex) + ".");
		return -1;
	}

	if (this->keyframeCount == MAX_KEYFRAMES)
	{
		DebugLogError("Can't add anymore animation keyframes.");
		return -1;
	}

	EntityAnimationDefinitionKeyframeList &keyframeList = this->keyframeLists[this->keyframeListCount];
	keyframeList.keyframeCount++;

	EntityAnimationDefinitionKeyframe &keyframe = this->keyframes[this->keyframeCount];
	keyframe.textureAsset = std::move(textureAsset);
	keyframe.width = width;
	keyframe.height = height;

	const int keyframeIndex = this->keyframeCount;
	this->keyframeCount++;
	return keyframeIndex;
}
