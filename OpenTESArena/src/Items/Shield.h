#ifndef SHIELD_H
#define SHIELD_H

#include <memory>
#include <string>
#include <vector>

#include "Armor.h"
#include "HeavyArmorMaterial.h"

// Shields are distinct from BodyArmor because their only ArmorMaterialType is plate, 
// since they are only designed to be metallic.

// It's a bit more clunky to get the metal than with weapons for example, because the
// metal depends on the shield's material, not just the shield itself.

// The original Arena does indeed have shields with metal (i.e., Elven or Adamantium),
// and also shields with no material (i.e., just "Round Shield"), which doesn't make 
// much sense. That's why I'm giving all shields a metal type in the remake.

class ShieldArtifactData;

enum class BodyPartName;
enum class MetalType;
enum class ShieldType;

class Shield : public Armor // Metallic goes through HeavyArmorMaterial.
{
private:
	std::unique_ptr<HeavyArmorMaterial> armorMaterial;
	ShieldType shieldType;
public:
	// Full constructor (intended for clone()).
	Shield(ShieldType shieldType, MetalType metalType,
		const ShieldArtifactData *artifactData);

	// Shield constructor for a shield type and metal type.
	Shield(ShieldType shieldType, MetalType metalType);

	// Shield artifact constructor.
	Shield(const ShieldArtifactData *artifactData);
	virtual ~Shield() = default;

	virtual std::unique_ptr<Item> clone() const override;

	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	ShieldType getShieldType() const;
	std::string typeToString() const;
	virtual ArmorType getArmorType() const override;
	virtual const ArmorMaterial *getArmorMaterial() const override;
	virtual std::vector<BodyPartName> getProtectedBodyParts() const override;
	virtual int getArmorRating() const override;
};

#endif
