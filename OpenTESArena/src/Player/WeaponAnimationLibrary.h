#pragma once

#include <unordered_map>

#include "components/utilities/Singleton.h"

#include "WeaponAnimation.h"

class TextureManager;

struct ExeData;

class WeaponAnimationLibrary : public Singleton<WeaponAnimationLibrary>
{
private:
	std::unordered_map<WeaponAnimationDefinitionID, WeaponAnimationDefinition> animDefs;
public:
	void init(const ExeData &exeData, TextureManager &textureManager);

	const WeaponAnimationDefinition &getDefinition(WeaponAnimationDefinitionID weaponAnimDefID) const;
};
