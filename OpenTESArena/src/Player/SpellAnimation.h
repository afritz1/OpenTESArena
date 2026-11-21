#pragma once

#include "../Assets/TextureAsset.h"

#include "components/utilities/Buffer.h"

enum class SpellAnimationType
{
	Fire = 0,
	Ice = 1,
	Acid = 2,
	Shock = 3,
	Magic = 4,
	Smoke = 5 // Not present in original animations list. Original game mistakenly uses acid instead.
};

struct SpellAnimationDefinition
{
	SpellAnimationType type;
	int width;
	int height;
	Buffer<TextureAsset> textureAssets;

	SpellAnimationDefinition();
};

using SpellAnimationDefinitionID = int;

struct SpellAnimationInstance
{
	static constexpr double TARGET_SECONDS = 1.0;

	SpellAnimationDefinitionID defID;
	double currentSeconds;

	SpellAnimationInstance();

	void init(SpellAnimationDefinitionID defID, double currentSeconds);

	bool isFinished() const;

	void update(double dt);
};
