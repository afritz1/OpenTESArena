#include <cstdio>

#include "EntityAnimationDefinition.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

EntityAnimationDefinition::Keyframe::Keyframe(double width, double height)
{
	this->width = width;
	this->height = height;
}

double EntityAnimationDefinition::Keyframe::getWidth() const
{
	return this->width;
}

double EntityAnimationDefinition::Keyframe::getHeight() const
{
	return this->height;
}

EntityAnimationDefinition::State::State(double totalSeconds, bool loop, bool flipped)
{
	this->totalSeconds = totalSeconds;
	this->loop = loop;
	this->flipped = flipped;
}

BufferView<const EntityAnimationDefinition::Keyframe> EntityAnimationDefinition::State::getKeyframes() const
{
	return BufferView<const Keyframe>(this->keyframes.data(), static_cast<int>(this->keyframes.size()));
}

double EntityAnimationDefinition::State::getTotalSeconds() const
{
	return this->totalSeconds;
}

bool EntityAnimationDefinition::State::isLooping() const
{
	return this->loop;
}

bool EntityAnimationDefinition::State::isFlipped() const
{
	return this->flipped;
}

void EntityAnimationDefinition::State::addKeyframe(Keyframe &&keyframe)
{
	this->keyframes.push_back(std::move(keyframe));
}

void EntityAnimationDefinition::State::clearKeyframes()
{
	this->keyframes.clear();
}

EntityAnimationDefinition::StateList::StateList(const char *name)
{
	DebugAssertMsg((name != nullptr) && (std::strlen(name) > 0), "State list must have a name.");
	std::snprintf(this->name.data(), this->name.size(), "%s", name);
}

const char *EntityAnimationDefinition::StateList::getName() const
{
	return this->name.data();
}

BufferView<const EntityAnimationDefinition::State> EntityAnimationDefinition::StateList::getStates() const
{
	return BufferView<const State>(this->states.data(), static_cast<int>(this->states.size()));
}

void EntityAnimationDefinition::StateList::addState(State &&state)
{
	this->states.push_back(std::move(state));
}

void EntityAnimationDefinition::StateList::clearStates()
{
	this->states.clear();
}

int EntityAnimationDefinition::getStateListCount() const
{
	return static_cast<int>(this->stateLists.size());
}

const EntityAnimationDefinition::StateList &EntityAnimationDefinition::getStateList(int index) const
{
	DebugAssertIndex(this->stateLists, index);
	return this->stateLists[index];
}

bool EntityAnimationDefinition::tryGetStateListIndex(const char *name, int *outStateIndex) const
{
	if ((name == nullptr) || (std::strlen(name) == 0))
	{
		return false;
	}

	const int stateListCount = static_cast<int>(this->stateLists.size());
	for (int i = 0; i < stateListCount; i++)
	{
		const StateList &stateList = this->stateLists[i];
		if (StringView::caseInsensitiveEquals(stateList.getName(), name))
		{
			*outStateIndex = i;
			return true;
		}
	}

	return false;
}

void EntityAnimationDefinition::addStateList(StateList &&stateList)
{
	this->stateLists.push_back(std::move(stateList));
}

void EntityAnimationDefinition::removeStateList(const char *name)
{
	int stateIndex;
	if (this->tryGetStateListIndex(name, &stateIndex))
	{
		this->stateLists.erase(this->stateLists.begin() + stateIndex);
	}
}

void EntityAnimationDefinition::clear()
{
	this->stateLists.clear();
}
