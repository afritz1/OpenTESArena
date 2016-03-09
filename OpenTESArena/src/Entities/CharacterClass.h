#ifndef CHARACTER_CLASS_H
#define CHARACTER_CLASS_H

#include <memory>
#include <string>
#include <vector>

#include "CharacterClassName.h"

enum class ArmorMaterialType;
enum class CharacterClassCategoryName;
enum class ShieldType;
enum class WeaponType;

class CharacterClass
{
private:
	CharacterClassName className;	
public:
	CharacterClass(CharacterClassName className);
	~CharacterClass();

	const CharacterClassName &getClassName() const;
	CharacterClassCategoryName getClassCategoryName() const;
	std::string toString() const;

	// startingBaseHealth()...
	// startingHealthDice()...

	std::vector<ArmorMaterialType> getAllowedArmors() const;
	std::vector<ShieldType> getAllowedShields() const;
	std::vector<WeaponType> getAllowedWeapons() const;
};

#endif