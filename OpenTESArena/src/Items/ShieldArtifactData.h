#ifndef SHIELD_ARTIFACT_DATA_H
#define SHIELD_ARTIFACT_DATA_H

#include <string>

#include "ArmorArtifactData.h"

enum class ArmorType;
enum class MetalType;
enum class ShieldType;

class ShieldArtifactData : public ArmorArtifactData
{
private:
	ShieldType shieldType;
	MetalType metalType;
public:
	ShieldArtifactData(const std::string &displayName,
		const std::string &flavorText, const std::vector<int> &provinceIDs, 
		ShieldType shieldType, MetalType metalType);
	virtual ~ShieldArtifactData() = default;

	virtual std::unique_ptr<ArtifactData> clone() const override;

	ShieldType getShieldType() const;
	MetalType getMetalType() const;

	virtual ArmorType getArmorType() const override;
};

#endif
