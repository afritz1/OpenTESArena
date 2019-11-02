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
		const EntityAnimationData *animationData;
		StateType stateType;
		double percentDone;
	public:
		Instance(const EntityAnimationData *animationData);

		const State &getState() const;
		int getKeyframeIndex() const;

		void setStateType(StateType stateType);
		void resetTime();

		void tick(double dt);
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
