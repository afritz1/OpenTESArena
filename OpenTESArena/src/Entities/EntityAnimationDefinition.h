#ifndef ENTITY_ANIMATION_DEFINITION_H
#define ENTITY_ANIMATION_DEFINITION_H

#include <optional>

#include "EntityAnimationUtils.h"
#include "../Assets/TextureAsset.h"

struct EntityAnimationDefinitionState
{
	char name[EntityAnimationUtils::NAME_LENGTH];
	double seconds;
	int keyframeListsIndex;
	int keyframeListCount;
	bool isLooping;

	bool operator==(const EntityAnimationDefinitionState &other) const;
	bool operator!=(const EntityAnimationDefinitionState &other) const;
};

struct EntityAnimationDefinitionKeyframeList
{
	int keyframesIndex;
	int keyframeCount;
	bool isMirrored;

	bool operator==(const EntityAnimationDefinitionKeyframeList &other) const;
	bool operator!=(const EntityAnimationDefinitionKeyframeList &other) const;
};

struct EntityAnimationDefinitionKeyframe
{
	TextureAsset textureAsset;
	double width, height;

	bool operator==(const EntityAnimationDefinitionKeyframe &other) const;
	bool operator!=(const EntityAnimationDefinitionKeyframe &other) const;
};

struct EntityAnimationDefinition
{
	static constexpr int MAX_STATES = 8;
	static constexpr int MAX_KEYFRAME_LISTS = 64;
	static constexpr int MAX_KEYFRAMES = 128;

	EntityAnimationDefinitionState states[MAX_STATES];
	int stateCount;

	EntityAnimationDefinitionKeyframeList keyframeLists[MAX_KEYFRAME_LISTS];
	int keyframeListCount;

	EntityAnimationDefinitionKeyframe keyframes[MAX_KEYFRAMES];
	int keyframeCount;

	EntityAnimationDefinition();

	bool operator==(const EntityAnimationDefinition &other) const;
	bool operator!=(const EntityAnimationDefinition &other) const;

	std::optional<int> tryGetStateIndex(const char *name) const;
	int getLinearizedKeyframeIndex(int stateIndex, int keyframeListIndex, int keyframeIndex) const;

	int addState(const char *name, double seconds, bool isLooping);
	int addKeyframeList(int stateIndex, bool isMirrored);
	int addKeyframe(int keyframeListIndex, TextureAsset &&textureAsset, double width, double height);
};

#endif
