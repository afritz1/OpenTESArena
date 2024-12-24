#ifndef WEAPON_ANIMATION_H
#define WEAPON_ANIMATION_H

#include <cstddef>

#include "../Assets/TextureAsset.h"

#include "WeaponAnimationUtils.h"

struct WeaponAnimationDefinitionState
{
	char name[WeaponAnimationUtils::MAX_NAME_LENGTH];
	double seconds;
	int framesIndex;
	int frameCount;
};

struct WeaponAnimationDefinitionFrame
{
	TextureAsset textureAsset;
	int width, height;
	int xOffset, yOffset;
};

struct WeaponAnimationDefinition
{
	static constexpr int MAX_STATES = 12;
	static constexpr int MAX_FRAMES = 64;

	WeaponAnimationDefinitionState states[MAX_STATES];
	int stateCount;

	WeaponAnimationDefinitionFrame frames[MAX_FRAMES];
	int frameCount;

	WeaponAnimationDefinition();

	bool tryGetStateIndex(const char *name, int *outIndex) const;

	int addState(const char *name, double seconds);
	int addFrame(int stateIndex, const TextureAsset &textureAsset, int width, int height, int xOffset, int yOffset);
};

struct WeaponAnimationInstance
{
	static constexpr int MAX_STATES = WeaponAnimationDefinition::MAX_STATES;

	// Cached data for ease of state switching.
	double targetSecondsList[MAX_STATES];

	double currentSeconds; // Updated every frame.
	double targetSeconds; // Updated when changing states.
	double progressPercent; // Updated every frame.
	int currentStateIndex; // Points into this weapon's animation def.
	int nextStateIndex; // Next state to transition to, otherwise loops current state.
	int stateCount;

	WeaponAnimationInstance();

	bool isLooping() const;

	void addState(double targetSeconds);

	void setStateIndex(int index);
	void setNextStateIndex(int index);
	void resetTime();
	void clear();
	void update(double dt);
};

#endif
