#ifndef ENTITY_ANIMATION_INSTANCE_H
#define ENTITY_ANIMATION_INSTANCE_H

#include <array>
#include <string>
#include <vector>

#include "EntityAnimationDefinition.h"
#include "EntityAnimationUtils.h"

#include "components/utilities/BufferView.h"

// Instance-specific animation data, references a shared animation definition.

class EntityAnimationInstance
{
public:
	class Keyframe
	{
	private:
		int textureID;
	public:
		Keyframe(int textureID);

		int getTextureID() const;
	};

	class State
	{
	private:
		std::vector<Keyframe> keyframes;
		std::string textureName; // Used for writing textures to the renderer.
	public:
		BufferView<const Keyframe> getKeyframes() const;
		const std::string &getTextureName() const;

		void addKeyframe(Keyframe &&keyframe);
		void clearKeyframes();
		void setTextureName(std::string &&textureName);
	};

	class StateList
	{
	private:
		std::vector<State> states;
	public:
		BufferView<const State> getStates() const;

		void addState(State &&state);
		void clearStates();
	};
private:
	std::vector<StateList> stateLists;
	double currentSeconds; // Seconds through current animation.
	int stateListIndex; // Determined by animation definition state lists.
	int animDefIndex; // Index into animation definitions.

	// @todo: other fancy stuff, like discriminated union for what kind of animation instance;
	// NPC weapon ID, townsperson generated texture ID, etc..
public:
	EntityAnimationInstance();

	int getStateListCount() const;
	BufferView<const StateList> getStateLists() const;
	double getCurrentSeconds() const;
	int getStateListIndex() const;
	int getDefinitionIndex() const;

	void addStateList(StateList &&stateList);
	void clearStateLists();
	void setStateListIndex(int index);
	void resetTime();
	void tick(double dt, const EntityAnimationDefinition::State &defState);
};

#endif
