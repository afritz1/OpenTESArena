#ifndef WEAPON_ANIMATION_LIBRARY_H
#define WEAPON_ANIMATION_LIBRARY_H

#include <unordered_map>

#include "components/utilities/Singleton.h"

#include "WeaponAnimation.h"

class ExeData;
class TextureManager;

class WeaponAnimationLibrary : public Singleton<WeaponAnimationLibrary>
{
private:
	std::unordered_map<int, WeaponAnimationDefinition> animDefs;
public:
	void init(const ExeData &exeData, TextureManager &textureManager);

	const WeaponAnimationDefinition &getDefinition(int animDefID) const;
};

#endif
