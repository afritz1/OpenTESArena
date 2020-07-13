#ifndef ENTITY_ANIMATION_DEFINITION_H
#define ENTITY_ANIMATION_DEFINITION_H

#include <array>
#include <vector>

#include "EntityAnimationUtils.h"

#include "components/utilities/BufferView.h"

// Shared entity animation data for a particular set of animation directions.

class EntityAnimationDefinition
{
public:
	class Keyframe
	{
	private:
		// Dimensions of flat in world space. Required for determining the size of the
		// flat on-screen for selection and rendering.
		double width, height;
	public:
		Keyframe(double width, double height);

		double getWidth() const;
		double getHeight() const;
	};

	class State
	{
	private:
		std::vector<Keyframe> keyframes;
		double totalSeconds; // Duration of state in seconds.
		bool loop;
		bool flipped;
	public:
		State(double totalSeconds, bool loop, bool flipped);

		BufferView<const Keyframe> getKeyframes() const;
		double getTotalSeconds() const;
		bool isLooping() const;
		bool isFlipped() const;

		void addKeyframe(Keyframe &&keyframe);
		void clearKeyframes();
	};

	// Each of a state list's entries are for a specific animation angle.
	// They are expected to be in clockwise order with respect to direction.
	class StateList
	{
	private:
		std::array<char, EntityAnimationUtils::NAME_LENGTH> name; // Idle, Attack, etc..
		std::vector<State> states;
	public:
		StateList(const char *name);

		const char *getName() const;
		BufferView<const State> getStates() const;

		void addState(State &&state);
		void clearStates();
	};
private:
	std::vector<StateList> stateLists;
	// @todo: should have a name so it can be found in a database. I.e. "Rat".
public:
	int getStateListCount() const;
	const StateList &getStateList(int index) const;
	bool tryGetStateListIndex(const char *name, int *outStateIndex) const;

	void addStateList(StateList &&stateList);
	void removeStateList(const char *name);
	void clear();
};

#endif
