#ifndef ENTITY_ANIMATION_DATA_H
#define ENTITY_ANIMATION_DATA_H

#include <vector>

#include "components/utilities/BufferView.h"

class EntityAnimationData
{
public:
	enum class StateType
	{
		Idle,
		Walking,
		Looking,
		Attacking,
		Dying,
		Dead
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
		double secondsPerFrame;
		StateType type;
		bool loop;
	public:
		State(StateType type, double secondsPerFrame, bool loop);

		StateType getType() const;
		bool getLoop() const;
		double getSecondsPerFrame() const;
		BufferView<const Keyframe> getKeyframes() const;

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

		const State &getState(const EntityAnimationData &animationData) const;
		int getKeyframeIndex(const EntityAnimationData &animationData) const;

		void setStateType(StateType stateType);

		void resetTime();
		void reset();

		void tick(double dt, const EntityAnimationData &animationData);
	};
private:
	std::vector<State> states;

	const State *findState(StateType stateType) const;
public:
	void addState(State &&state);
	void removeState(StateType stateType);
	void clear();
};

#endif
