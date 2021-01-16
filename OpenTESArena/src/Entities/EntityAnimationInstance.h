#ifndef ENTITY_ANIMATION_INSTANCE_H
#define ENTITY_ANIMATION_INSTANCE_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "EntityAnimationDefinition.h"
#include "EntityAnimationUtils.h"
#include "../Assets/TextureAssetReference.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/BufferView.h"

// Instance-specific animation data, references a shared animation definition.

class TextureBuilder;
class TextureManager;

class EntityAnimationInstance
{
public:
	class Keyframe
	{
	private:
		// @todo: use an entity texture handle from the renderer. This should be shareable between different
		// entities; for example, 100 chain armor male enemies with swords could all have this same handle.
		// In order to effectively handle enemy human weapons, each instance keyframe should be an allocated
		// renderer texture handle like EntityTextureID with the weapon texture drawn on top of the human texture.
		// Don't complicate anything by having "multiple layers" in a keyframe. It's just one texture. There
		// could be sharing of that combination (i.e. medium armor male enemy with sword) done beforehand and
		// the keyframe would still get that texture handle.
	public:
		// Gets the raw texture builder handle for this keyframe (does not protect from dangling pointers).
		// @todo: eventually return renderer texture handle instead and maybe don't pass anim def keyframe. The
		// entity animation definition needs its own entity texture handles allocated as "the go-to ones" if there
		// is nothing interesting to set as the override handle in this instance keyframe.
		const TextureBuilder &getTextureBuilderHandle(const EntityAnimationDefinition::Keyframe &defKeyframe,
			TextureManager &textureManager) const;
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

	struct CitizenParams
	{
		// Each citizen has a unique palette instead of unique textures for memory savings. It was found
		// through testing that hardly any citizen instances share textures due to variations in their
		// random palette. As a result, citizen textures will need to be 8-bit.
		Palette palette;
	};
private:
	std::vector<State> states;
	std::unique_ptr<CitizenParams> citizenParams; // Extra data for citizens.
	double currentSeconds; // Seconds through current state.
	int stateIndex; // Active state, also usable with animation definition states.
public:
	EntityAnimationInstance();
	EntityAnimationInstance(const EntityAnimationInstance &other);

	EntityAnimationInstance &operator=(const EntityAnimationInstance &other);

	int getStateCount() const;
	const State &getState(int index) const;
	const CitizenParams *getCitizenParams() const;
	double getCurrentSeconds() const;
	int getStateIndex() const;

	void addState(State &&state);
	void clearStates();

	void setCitizenParams(std::unique_ptr<CitizenParams> &&citizenParams);

	// Sets the active state index shared between this instance and its definition.
	void setStateIndex(int index);

	void reset();
	void resetTime();

	// Animates the instance by delta time and loops if the total seconds is exceeded.
	void tick(double dt, double totalSeconds, bool looping);
};

#endif
