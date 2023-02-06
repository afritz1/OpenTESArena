#ifndef ENTITY_ANIMATION_DEFINITION_H
#define ENTITY_ANIMATION_DEFINITION_H

#include <array>
#include <optional>
#include <vector>
#include <string>

#include "EntityAnimationUtils.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/BufferView.h"

// Shared entity animation data for a particular set of animation directions.

class EntityAnimationDefinition
{
public:
	class Keyframe
	{
	private:
		TextureAsset textureAsset;

		// Dimensions of flat in world space. Required for determining the size of the
		// flat on-screen for selection and rendering.
		double width, height;
	public:
		Keyframe(TextureAsset &&textureAsset, double width, double height);

		bool operator==(const Keyframe &other) const;
		bool operator!=(const Keyframe &other) const;

		const TextureAsset &getTextureAsset() const;
		double getWidth() const;
		double getHeight() const;
	};

	class KeyframeList
	{
	private:
		std::vector<Keyframe> keyframes;
		bool flipped;
	public:
		KeyframeList();

		void init(bool flipped);

		bool operator==(const KeyframeList &other) const;
		bool operator!=(const KeyframeList &other) const;

		int getKeyframeCount() const;
		const Keyframe &getKeyframe(int index) const;
		bool isFlipped() const;

		void addKeyframe(Keyframe &&keyframe);
		void clearKeyframes();
	};

	// Each of a state's entries are for a specific animation angle.
	// They are expected to be in clockwise order with respect to direction.
	class State
	{
	private:
		std::array<char, EntityAnimationUtils::NAME_LENGTH> name; // Idle, Attack, etc..
		std::vector<KeyframeList> keyframeLists; // Each list occupies a slice of 360 degrees.
		double totalSeconds; // Duration of state in seconds.
		bool loop;
	public:
		State();

		void init(const char *name, double totalSeconds, bool loop);

		bool operator==(const State &other) const;
		bool operator!=(const State &other) const;

		const char *getName() const;
		int getKeyframeListCount() const;
		const KeyframeList &getKeyframeList(int index) const;
		double getTotalSeconds() const;
		bool isLooping() const;

		void addKeyframeList(KeyframeList &&keyframeList);
		void clearKeyframeLists();
	};
private:
	std::vector<State> states; // Idle, Attack, etc..
public:
	bool operator==(const EntityAnimationDefinition &other) const;
	bool operator!=(const EntityAnimationDefinition &other) const;

	int getStateCount() const;
	const State &getState(int index) const;
	std::optional<int> tryGetStateIndex(const char *name) const;

	int getTotalKeyframeCount() const;
	int getLinearizedKeyframeIndex(int stateIndex, int keyframeListIndex, int keyframeIndex) const;

	void addState(State &&state);
	void removeState(const char *name);
	void clear();
};

#endif
