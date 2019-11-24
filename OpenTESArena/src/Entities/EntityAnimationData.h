#ifndef ENTITY_ANIMATION_DATA_H
#define ENTITY_ANIMATION_DATA_H

#include <vector>

#include "components/utilities/BufferView.h"

class EntityAnimationData
{
public:
	// @todo: this should probably be a discriminated union, I guess? So we can have static and dynamic
	// state types in the same class without needing derived classes.
	// @todo: probably have a "returnState" mapping for non-looping states.
	enum class StateType
	{
		Idle,
		Look,
		Walk,
		Attack,
		Death
	};

	class Keyframe
	{
	private:
		// Dimensions of flat in world space. Required for determining the size of
		// the flat on-screen for selection and rendering.
		double width, height;

		int textureID;
	public:
		Keyframe(double width, double height, int textureID);

		double getWidth() const;
		double getHeight() const;
		int getTextureID() const;
	};

	class State
	{
	private:
		std::vector<Keyframe> keyframes;
		std::string textureName; // Used for writing textures to the renderer.
		double secondsPerFrame;
		StateType type;
		bool loop;
		bool flipped;
	public:
		State(StateType type, double secondsPerFrame, bool loop, bool flipped);

		StateType getType() const;
		bool isFlipped() const;
		bool getLoop() const;
		double getSecondsPerFrame() const;
		const std::string &getTextureName() const;
		BufferView<const Keyframe> getKeyframes() const;

		void setTextureName(std::string &&textureName);
		void addKeyframe(Keyframe &&keyframe);
		void clearKeyframes();
	};

	class Instance
	{
	private:
		StateType stateType;
		double percentDone;
	public:
		Instance();

		// Animation data is passed by reference because its EntityData owner is
		// allocated on the heap and can become dangling if a pointer is stored here.

		const std::vector<State> &getStateList(const EntityAnimationData &animationData) const;
		int getKeyframeIndex(int stateIndex, const EntityAnimationData &animationData) const;

		void setStateType(StateType stateType);

		void resetTime();
		void reset();

		void tick(double dt, const EntityAnimationData &animationData);
	};
private:
	// Each state list contains one or more states, intended for directional animations.
	// The renderer decides which state to display depending on relative angle, etc..
	// State lists are expected to be in clockwise order with respect to directions.
	std::vector<std::vector<State>> stateLists;

	const std::vector<State> *findStateList(StateType stateType) const;
public:
	void addStateList(std::vector<State> &&stateList);
	void removeStateList(StateType stateType);
	void clear();
};

#endif
