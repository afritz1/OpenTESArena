#ifndef WEAPON_H
#define WEAPON_H

#include <memory>
#include <string>

#include "Item.h"
#include "Metallic.h"

class ExeData;
class WeaponArtifactData;

enum class WeaponHandCount;
enum class WeaponRangeType;

class Weapon : public Item, public Metallic
{
private:
	int weaponID;
	std::string weaponName;
	std::unique_ptr<WeaponArtifactData> artifactData;

	Weapon(int weaponID, MetalType metalType,
		const WeaponArtifactData *artifactData, const ExeData &exeData);
public:
	// Full constructor (intended for clone()).
	Weapon(int weaponID, const std::string &weaponName, MetalType metalType,
		const WeaponArtifactData *artifactData);

	// Weapon constructor for a weapon type and metal type.
	Weapon(int weaponID, MetalType metalType, const ExeData &exeData);

	// Weapon artifact constructor.
	Weapon(const WeaponArtifactData *artifactData, const ExeData &exeData);
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
