#ifndef BODY_ARMOR_ARTIFACT_DATA_H
#define BODY_ARMOR_ARTIFACT_DATA_H

#include <memory>
#include <vector>

#include "ArmorArtifactData.h"

class ArmorMaterial;

enum class ArmorType;
enum class BodyPartName;

class BodyArmorArtifactData : public ArmorArtifactData
{
private:
	std::unique_ptr<ArmorMaterial> armorMaterial;
	BodyPartName partName;
public:
	BodyArmorArtifactData(const std::string &displayName,
		const std::string &flavorText, const std::vector<int> &provinceIDs,
		const ArmorMaterial *armorMaterial, BodyPartName partName);
	virtual ~BodyArmorArtifactData();

	virtual std::unique_ptr<ArtifactData> clone() const override;

	BodyPartName getBodyPartName() const;
	const ArmorMaterial *getArmorMaterial() const;

	// The armor type is found by using the body part name.
	virtual ArmorType getArmorType() const override;
};

#endif
