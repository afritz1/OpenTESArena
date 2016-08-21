#ifndef CHARACTER_CLASS_H
#define CHARACTER_CLASS_H

#include <cstdint>
#include <string>
#include <vector>

enum class ArmorMaterialType;
enum class CharacterClassCategoryName;
enum class ShieldType;
enum class WeaponType;

class CharacterClass
{
private:
	std::string displayName;
	CharacterClassCategoryName categoryName;
	bool castsMagic;
	int32_t startingHealth, healthDice;
	std::vector<ArmorMaterialType> allowedArmors;
	std::vector<ShieldType> allowedShields;
	std::vector<WeaponType> allowedWeapons;
public:
	CharacterClass(const std::string &displayName, 
		CharacterClassCategoryName categoryName, bool castsMagic, 
		int32_t startingHealth, int32_t healthDice, 
		const std::vector<ArmorMaterialType> &allowedArmors,
		const std::vector<ShieldType> &allowedShields,
		const std::vector<WeaponType> &allowedWeapons);
	~CharacterClass();

	const std::string &getDisplayName() const;
	CharacterClassCategoryName getClassCategoryName() const;
	bool canCastMagic() const;
	int32_t getStartingHealth() const;
	int32_t getHealthDice() const;
	const std::vector<ArmorMaterialType> &getAllowedArmors() const;
	const std::vector<ShieldType> &getAllowedShields() const;
	const std::vector<WeaponType> &getAllowedWeapons() const;
};

#endif
