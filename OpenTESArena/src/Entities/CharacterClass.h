#ifndef CHARACTER_CLASS_H
#define CHARACTER_CLASS_H

#include <string>
#include <vector>

enum class ArmorMaterialType;
enum class CharacterClassCategoryName;
enum class ShieldType;

class CharacterClass
{
private:
	std::string name, preferredAttributes;
	std::vector<ArmorMaterialType> allowedArmors;
	std::vector<ShieldType> allowedShields;
	std::vector<int> allowedWeapons;
	CharacterClassCategoryName categoryName;
	double lockpicking; // Lockpick effectiveness percent.
	int healthDie; // Die used in character generation (d8, d20, ...).
	int initialExperienceCap; // Experience to get from level 1 to 2.
	int classIndex; // Index in the classes array.
	bool mage, thief, criticalHit;
public:
	CharacterClass(const std::string &name, const std::string &preferredAttributes,
		const std::vector<ArmorMaterialType> &allowedArmors,
		const std::vector<ShieldType> &allowedShields,
		const std::vector<int> &allowedWeapons,
		CharacterClassCategoryName categoryName, double lockpicking, int healthDie,
		int initialExperienceCap, int classIndex, bool mage, bool thief, bool criticalHit);
	~CharacterClass();

	const std::string &getName() const;
	const std::string &getPreferredAttributes() const;
	const std::vector<ArmorMaterialType> &getAllowedArmors() const;
	const std::vector<ShieldType> &getAllowedShields() const;
	const std::vector<int> &getAllowedWeapons() const;
	CharacterClassCategoryName getCategoryName() const;
	double getLockpicking() const;
	int getHealthDie() const;
	int getInitialExperienceCap() const;
	int getClassIndex() const;
	bool canCastMagic() const;
	bool isThief() const;
	bool hasCriticalHit() const;

	// Gets the experience required for some given level.
	int getExperienceCap(int level) const;
};

#endif
