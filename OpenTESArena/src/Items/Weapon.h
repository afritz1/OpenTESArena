#ifndef WEAPON_H
#define WEAPON_H

#include <memory>
#include <string>

#include "Item.h"
#include "Metallic.h"

class ExeStrings;
class WeaponArtifactData;

enum class WeaponHandCount;
enum class WeaponRangeType;

class Weapon : public Item, public Metallic
{
private:
	int weaponID;
	std::string weaponName;
	std::unique_ptr<WeaponArtifactData> artifactData;

	// Constructor for clone().
	Weapon(int weaponID, const std::string &weaponName, MetalType metalType,
		const WeaponArtifactData *artifactData);

	Weapon(int weaponID, MetalType metalType,
		const WeaponArtifactData *artifactData, const ExeStrings &exeStrings);
public:
	// Weapon constructor for a weapon type and metal type.
	Weapon(int weaponID, MetalType metalType, const ExeStrings &exeStrings);

	// Weapon artifact constructor.
	Weapon(const WeaponArtifactData *artifactData, const ExeStrings &exeStrings);
	virtual ~Weapon();

	virtual std::unique_ptr<Item> clone() const override;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	int getWeaponID() const;
	const std::string &getWeaponName() const;
	WeaponHandCount getHandCount() const;
	WeaponRangeType getWeaponRangeType() const;
	int getBaseMinDamage() const;
	int getBaseMaxDamage() const;
};

#endif
