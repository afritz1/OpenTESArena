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
enum class BodyArmorArtifactName;

class BodyArmor : public Armor
{
private:
	std::unique_ptr<BodyArmorArtifactData> artifactData;
	std::unique_ptr<ArmorMaterial> armorMaterial;
	std::unique_ptr<BodyPart> part;
public:
	// The body armor object takes ownership of the armor material. This was more neat
	// than having multiple constructors for armor material type, metal type, etc..
	BodyArmor(BodyPartName partName, std::unique_ptr<ArmorMaterial> armorMaterial);

	// Body armor artifact constructor.
	BodyArmor(BodyArmorArtifactName artifactName);

	BodyArmor(const BodyArmor &bodyArmor);

	virtual ~BodyArmor();

	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const BodyArmorArtifactData *getArtifactData() const;

	const BodyPart &getBodyPart() const;
	std::string typeToString() const;
	virtual ArmorType getArmorType() const override;
	virtual const ArmorMaterial *getArmorMaterial() const override;
	virtual std::vector<BodyPartName> getProtectedBodyParts() const override;
	virtual int getArmorRating() const override;
};

#endif
