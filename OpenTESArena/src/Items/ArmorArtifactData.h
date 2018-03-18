#ifndef ARMOR_ARTIFACT_DATA_H
#define ARMOR_ARTIFACT_DATA_H

#include "ArtifactData.h"

// This abstract class determines whether an armor artifact is a body armor artifact
// or a shield artifact.

enum class ArmorType;

class ArmorArtifactData : public ArtifactData
{
public:
	ArmorArtifactData(const std::string &displayName,
		const std::string &flavorText, const std::vector<int> &provinceIDs);
	virtual ~ArmorArtifactData() = default;

	virtual std::unique_ptr<ArtifactData> clone() const override = 0;

	virtual ItemType getItemType() const override;

	virtual ArmorType getArmorType() const = 0;
};

#endif
