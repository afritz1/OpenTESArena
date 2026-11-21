#pragma once

#include <vector>

#include "SpellAnimation.h"

#include "components/utilities/Singleton.h"

struct ExeData;

class TextureManager;

struct SpellAnimationLibrary : Singleton<SpellAnimationLibrary>
{
	std::vector<SpellAnimationDefinition> defs;

	void init(const ExeData &exeData, TextureManager &textureManager);

	const SpellAnimationDefinition &getDefinition(SpellAnimationDefinitionID id) const;
	SpellAnimationDefinitionID getDefinitionIdByType(SpellAnimationType type) const;
};
