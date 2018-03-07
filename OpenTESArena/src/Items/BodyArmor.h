#ifndef BODY_ARMOR_H
#define BODY_ARMOR_H

#include <memory>
#include <vector>

#include "Armor.h"

// This is one of the concrete implementations of the Armor class, for armor on
// the body (helm, cuirass, boots). This is necessary because shields are abstract 
// without their ShieldType, and it would make no sense for the Armor class to have 
// a ShieldType.

class ArmorMaterial;
class BodyArmorArtifactData;
class BodyPart;

enum class BodyPartName;

class BodyArmor : public Armor
{
private:
	std::unique_ptr<ArmorMaterial> armorMaterial;
	BodyPartName partName;
public:
	// Full constructor (intended for clone()).
	BodyArmor(BodyPartName partName, const ArmorMaterial *armorMaterial,
		const BodyArmorArtifactData *artifactData);

	// Body armor constructor.
	BodyArmor(BodyPartName partName, const ArmorMaterial *armorMaterial);

	// Body armor artifact constructor.
	BodyArmor(const BodyArmorArtifactData *artifactData);
	virtual ~BodyArmor();

	virtual std::unique_ptr<Item> clone() const override;

	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	BodyPartName getPartName() const;
	std::string typeToString() const;

	virtual ArmorType getArmorType() const override;
	virtual const ArmorMaterial *getArmorMaterial() const override;
	virtual std::vector<BodyPartName> getProtectedBodyParts() const override;
	virtual int getArmorRating() const override;
};

#endif
