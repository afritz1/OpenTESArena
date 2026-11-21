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

	int findDefinitionIndex(SpellAnimationType type);
};
