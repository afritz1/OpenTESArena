#ifndef WEAPON_H
#define WEAPON_H

#include <memory>
#include <string>

#include "Item.h"
#include "Metallic.h"
#include "WeaponType.h"

// It seems common place in the original game to have weapons with no material.
// What is a plain old "dagger", then? Where does its base damage come from? 
// I'm designing all weapons in the remake to have a metal type in order to 
// alleviate this issue. That includes bows, because designing a new "wood"
// abstract material is a bit too far outside the design scope.

class WeaponArtifactData;

enum class WeaponArtifactName;
enum class WeaponHandCount;
enum class WeaponRangeName;

class Weapon : public Item, public Metallic
{
private:
	std::unique_ptr<WeaponArtifactData> artifactData;
	WeaponType weaponType;
public:
	// Weapon constructor for a weapon type and metal type.
	Weapon(WeaponType weaponType, MetalType metalType);

	// Weapon artifact constructor.
	Weapon(WeaponArtifactName artifactName);

	Weapon(const Weapon &weapon);

	virtual ~Weapon();

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const WeaponArtifactData *getArtifactData() const;

	const WeaponType &getWeaponType() const;
	WeaponHandCount getHandCount() const;
	int getBaseMinDamage() const;
	int getBaseMaxDamage() const;
	WeaponRangeName getWeaponRangeName() const;
	std::string typeToString() const;
};

#endif