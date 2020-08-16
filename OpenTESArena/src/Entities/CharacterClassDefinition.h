#ifndef CHARACTER_CLASS_DEFINITION_H
#define CHARACTER_CLASS_DEFINITION_H

#include <optional>
#include <string>
#include <vector>

class CharacterClassDefinition
{
public:
	// Mage, thief, etc..
	using CategoryID = int;
private:
	std::string name;
	CategoryID categoryID;
	std::string preferredAttributes; // Description in character creation.
	std::vector<int> allowedArmors; // 0 = leather, 1 = chain, etc..
	std::vector<int> allowedShields; // 0 = buckler, 1 = round shield, etc..
	std::vector<int> allowedWeapons; // 0 = staff, 1 = sword, etc..
	bool castsMagic;
	int healthDie; // d8, d20, etc..
	int initialExpCap;
	double lockpickPercent; // Lockpick effectiveness percent.
	bool criticalHit;
	std::optional<int> originalClassIndex; // Set if derived from original game.
public:
	CharacterClassDefinition();

	void init(std::string &&name, CategoryID categoryID, std::string &&preferredAttributes,
		const int *allowedArmors, int allowedArmorCount,
		const int *allowedShields, int allowedShieldCount,
		const int *allowedWeapons, int allowedWeaponCount,
		bool castsMagic, int healthDie, int initialExpCap, double lockpickPercent,
		bool criticalHit, const std::optional<int> &originalClassIndex);

	// Gets the experience required for the given level with some initial experience cap.
	static int getExperienceCap(int level, int initialExpCap);

	const std::string &getName() const;
	CategoryID getCategoryID() const;
	const std::string &getPreferredAttributes() const;
	int getAllowedArmorCount() const;
	int getAllowedShieldCount() const;
	int getAllowedWeaponCount() const;
	int getAllowedArmor(int index) const;
	int getAllowedShield(int index) const;
	int getAllowedWeapon(int index) const;
	bool canCastMagic() const;
	int getHealthDie() const;
	int getInitialExperienceCap() const;
	double getLockpickPercent() const;
	bool hasCriticalHit() const;
	const std::optional<int> &getOriginalClassIndex() const;
};

#endif
