#ifndef WEAPON_H
#define WEAPON_H

#include <memory>
#include <string>

#include "Item.h"
#include "Metallic.h"

// It seems common place in the original game to have weapons with no material.
// What is a plain old "dagger", then? Where does its base damage come from? 
// I'm designing all weapons in the remake to have a metal type in order to 
// alleviate this issue. That includes bows, because designing a new "wood"
// abstract material is a bit too far outside the design scope.

class WeaponArtifactData;

enum class WeaponHandCount;
enum class WeaponRangeName;
enum class WeaponType;

class Weapon : public Item, public Metallic
{
private:
	std::unique_ptr<WeaponArtifactData> artifactData;
	WeaponType weaponType;

	Weapon(WeaponType weaponType, MetalType metalType,
		const WeaponArtifactData *artifactData);
public:
	// Weapon constructor for a weapon type and metal type.
	Weapon(WeaponType weaponType, MetalType metalType);

	// Weapon artifact constructor.
	Weapon(const WeaponArtifactData *artifactData);
	virtual ~Weapon();

	virtual std::unique_ptr<Item> clone() const override;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int32_t getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	WeaponType getWeaponType() const;
	WeaponHandCount getHandCount() const;
	WeaponRangeName getWeaponRangeName() const;
	int32_t getBaseMinDamage() const;
	int32_t getBaseMaxDamage() const;
	std::string typeToString() const;
};

#endif
