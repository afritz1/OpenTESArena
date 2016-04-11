#ifndef BODY_ARMOR_ARTIFACT_DATA_H
#define BODY_ARMOR_ARTIFACT_DATA_H

#include <memory>
#include <string>

#include "BodyArmorArtifactName.h"
#include "ArtifactData.h"

class ArmorMaterial;

enum class BodyPartName;
enum class MetalType;

class BodyArmorArtifactData : public ArtifactData
{
private:
	BodyArmorArtifactName artifactName;
public:
	BodyArmorArtifactData(BodyArmorArtifactName artifactName);
	virtual ~BodyArmorArtifactData();

	const BodyArmorArtifactName &getArtifactName() const;
	BodyPartName getBodyPartName() const;
	std::unique_ptr<ArmorMaterial> getArmorMaterial() const;
	virtual ArtifactName getParentArtifactName() const override;
	virtual std::string getDisplayName() const override;
	virtual std::string getFlavorText() const override;
};

#endif
