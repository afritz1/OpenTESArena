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
		const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
		const ShieldType &shieldType, const MetalType &metalType);
	virtual ~ShieldArtifactData();

	virtual std::unique_ptr<ArtifactData> clone() const override;

	const ShieldType &getShieldType() const;
	const MetalType &getMetalType() const;

	virtual ArmorType getArmorType() const override;
};

#endif
