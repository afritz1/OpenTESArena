#ifndef ENTITY_ANIMATION_INSTANCE_H
#define ENTITY_ANIMATION_INSTANCE_H

#include <array>
#include <string>
#include <vector>

#include "EntityAnimationDefinition.h"
#include "EntityAnimationUtils.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/BufferView.h"

// Instance-specific animation data, references a shared animation definition.

class Image;
class TextureInstanceManager;
class TextureManager;

class EntityAnimationInstance
{
public:
	class Keyframe
	{
	private:
		ImageID overrideImageID;
		ImageInstanceID overrideImageInstID;
	public:
		Keyframe();

		static Keyframe makeFromImage(ImageID overrideImageID);
		static Keyframe makeFromImageInstance(ImageInstanceID overrideImageInstID);

		// Gets the raw image handle for this keyframe (does not protect from dangling pointers).
		// It checks the generated instance ID, then the regular ID, then the definition's ID.
		const Image &getImageHandle(const EntityAnimationDefinition::Keyframe &defKeyframe,
			const TextureManager &textureManager, const TextureInstanceManager &textureInstManager) const;
	};

	class KeyframeList
	{
	private:
		std::vector<Keyframe> keyframes;
	public:
		int getKeyframeCount() const;
		const Keyframe &getKeyframe(int index) const;

		void addKeyframe(Keyframe &&keyframe);
		void clearKeyframes();
	};

	class State
	{
	private:
		std::vector<KeyframeList> keyframeLists;
	public:
		int getKeyframeListCount() const;
		const KeyframeList &getKeyframeList(int index) const;

		void addKeyframeList(KeyframeList &&keyframeList);
		void clearKeyframeLists();
	};
private:
	std::vector<State> states;
	double currentSeconds; // Seconds through current state.
	int stateIndex; // Active state, also usable with animation definition states.

	// @todo: other fancy stuff, like discriminated union for what kind of animation instance;
	// NPC weapon ID, townsperson generated texture ID, etc..
public:
	EntityAnimationInstance();

	int getStateCount() const;
	const State &getState(int index) const;
	double getCurrentSeconds() const;
	int getStateIndex() const;

	void addState(State &&state);
	void clearStates();

	// Sets the active state index shared between this instance and its definition.
	void setStateIndex(int index);

	void reset();
	void resetTime();

	// Animates the instance by delta time and loops if the total seconds is exceeded.
	void tick(double dt, double totalSeconds, bool looping);
};

#endif
