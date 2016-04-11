#ifndef WEAPON_ARTIFACT_DATA_H
#define WEAPON_ARTIFACT_DATA_H

#include <string>

#include "ArtifactData.h"
#include "WeaponArtifactName.h"

// Look at AccessoryArtifact... and BodyArmorArtifact... 

enum class MetalType;
enum class WeaponType;

class WeaponArtifactData : public ArtifactData
{
private:
	WeaponArtifactName artifactName;
public:
	WeaponArtifactData(WeaponArtifactName artifactName);
	virtual ~WeaponArtifactData();

	const WeaponArtifactName &getArtifactName() const;
	WeaponType getWeaponType() const;
	MetalType getMetalType() const;

	virtual ArtifactName getParentArtifactName() const override;
	virtual std::string getDisplayName() const override;
	virtual std::string getFlavorText() const override;
};

#endif
