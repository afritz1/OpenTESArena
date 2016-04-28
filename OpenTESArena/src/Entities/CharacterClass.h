#ifndef CHARACTER_CLASS_H
#define CHARACTER_CLASS_H

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
	int startingHealth, healthDice;
	std::vector<ArmorMaterialType> allowedArmors;
	std::vector<ShieldType> allowedShields;
	std::vector<WeaponType> allowedWeapons;
public:
	CharacterClass(const std::string &displayName, 
		CharacterClassCategoryName categoryName, bool castsMagic, 
		int startingHealth, int healthDice, 
		const std::vector<ArmorMaterialType> &allowedArmors,
		const std::vector<ShieldType> &allowedShields,
		const std::vector<WeaponType> &allowedWeapons);
	~CharacterClass();

	const std::string &getDisplayName() const;
	const CharacterClassCategoryName &getClassCategoryName() const;
	const bool &canCastMagic() const;
	const int &getStartingHealth() const;
	const int &getHealthDice() const;
	const std::vector<ArmorMaterialType> &getAllowedArmors() const;
	const std::vector<ShieldType> &getAllowedShields() const;
	const std::vector<WeaponType> &getAllowedWeapons() const;
};

#endif
