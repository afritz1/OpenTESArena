#include <cstdio>
#include <cstring>

#include "EntityAnimationDefinition.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

bool EntityAnimationDefinitionState::operator==(const EntityAnimationDefinitionState &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (std::strncmp(this->name, other.name, std::size(this->name)) != 0)
	{
		return false;
	}

	if (this->seconds != other.seconds)
	{
		return false;
	}

	if (this->keyframeListsIndex != other.keyframeListsIndex)
	{
		return false;
	}

	if (this->keyframeListCount != other.keyframeListCount)
	{
		return false;
	}

	if (this->isLooping != other.isLooping)
	{
		return false;
	}

	return true;
}

bool EntityAnimationDefinitionState::operator!=(const EntityAnimationDefinitionState &other) const
{
	return !(*this == other);
}

bool EntityAnimationDefinitionKeyframeList::operator==(const EntityAnimationDefinitionKeyframeList &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->keyframesIndex != other.keyframesIndex)
	{
		return false;
	}

	if (this->keyframeCount != other.keyframeCount)
	{
		return false;
	}

	if (this->isFlipped != other.isFlipped)
	{
		return false;
	}
	
	return true;
}

bool EntityAnimationDefinitionKeyframeList::operator!=(const EntityAnimationDefinitionKeyframeList &other) const
{
	return !(*this == other);
}

bool EntityAnimationDefinitionKeyframe::operator==(const EntityAnimationDefinitionKeyframe &other) const
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

bool EntityAnimationDefinitionKeyframe::operator!=(const EntityAnimationDefinitionKeyframe &other) const
{
	return !(*this == other);
}

EntityAnimationDefinition::EntityAnimationDefinition()
{
	this->stateCount = 0;
	this->keyframeListCount = 0;
	this->keyframeCount = 0;
}

bool EntityAnimationDefinition::operator==(const EntityAnimationDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->stateCount != other.stateCount)
	{
		return false;
	}

	if (this->keyframeListCount != other.keyframeListCount)
	{
		return false;
	}

	if (this->keyframeCount != other.keyframeCount)
	{
		return false;
	}

	for (int i = 0; i < this->stateCount; i++)
	{
		const EntityAnimationDefinitionState &aState = this->states[i];
		const EntityAnimationDefinitionState &bState = other.states[i];
		if (aState != bState)
		{
			return false;
		}
	}

	for (int i = 0; i < this->keyframeListCount; i++)
	{
		const EntityAnimationDefinitionKeyframeList &aKeyframeList = this->keyframeLists[i];
		const EntityAnimationDefinitionKeyframeList &bKeyframeList = other.keyframeLists[i];
		if (aKeyframeList != bKeyframeList)
		{
			return false;
		}
	}

	for (int i = 0; i < this->keyframeCount; i++)
	{
		const EntityAnimationDefinitionKeyframe &aKeyframe = this->keyframes[i];
		const EntityAnimationDefinitionKeyframe &bKeyframe = other.keyframes[i];
		if (aKeyframe != bKeyframe)
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

std::optional<int> EntityAnimationDefinition::tryGetStateIndex(const char *name) const
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

int EntityAnimationDefinition::getLinearizedKeyframeIndex(int stateIndex, int keyframeListIndex, int keyframeIndex) const
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

int EntityAnimationDefinition::addState(const char *name, double seconds, bool isLooping)
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

int EntityAnimationDefinition::addKeyframeList(int stateIndex, bool isFlipped)
{
	if ((stateIndex < 0) || (stateIndex >= this->stateCount))
	{
		DebugLogError("Invalid state index " + std::to_string(stateIndex) + ".");
		return -1;
	}

	if (this->keyframeListCount == MAX_KEYFRAME_LISTS)
	{
		DebugLogError("Can't add anymore animation keyframe lists.");
		return -1;
	}

	EntityAnimationDefinitionState &state = this->states[stateIndex];
	state.keyframeListCount++;

	EntityAnimationDefinitionKeyframeList &keyframeList = this->keyframeLists[this->keyframeListCount];
	keyframeList.keyframesIndex = this->keyframeCount;
	keyframeList.keyframeCount = 0;
	keyframeList.isFlipped = isFlipped;

	const int keyframeListIndex = this->keyframeListCount;
	this->keyframeListCount++;
	return keyframeListIndex;
}

int EntityAnimationDefinition::addKeyframe(int keyframeListIndex, TextureAsset &&textureAsset, double width, double height)
{
	if ((keyframeListIndex < 0) || (keyframeListIndex >= this->keyframeListCount))
	{
		DebugLogError("Invalid keyframe list index " + std::to_string(keyframeListIndex) + ".");
		return -1;
	}

	if (this->keyframeCount == MAX_KEYFRAMES)
	{
		DebugLogError("Can't add anymore animation keyframes.");
		return -1;
	}

	EntityAnimationDefinitionKeyframeList &keyframeList = this->keyframeLists[keyframeListIndex];
	keyframeList.keyframeCount++;

	EntityAnimationDefinitionKeyframe &keyframe = this->keyframes[this->keyframeCount];
	keyframe.textureAsset = std::move(textureAsset);
	keyframe.width = width;
	keyframe.height = height;

	const int keyframeIndex = this->keyframeCount;
	this->keyframeCount++;
	return keyframeIndex;
}
