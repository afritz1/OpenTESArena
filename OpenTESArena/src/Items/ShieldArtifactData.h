#ifndef SHIELD_ARTIFACT_DATA_H
#define SHIELD_ARTIFACT_DATA_H

#include <string>

#include "ArtifactData.h"
#include "ShieldArtifactName.h"

enum class MetalType;
enum class ShieldType;

class ShieldArtifactData : public ArtifactData
{
private:
	ShieldArtifactName artifactName;
public:
	ShieldArtifactData(ShieldArtifactName artifactName);
	virtual ~ShieldArtifactData();

	const ShieldArtifactName &getArtifactName() const;
	ShieldType getShieldType() const;
	MetalType getMetalType() const;

	virtual ArtifactName getParentArtifactName() const override;
	virtual std::string getDisplayName() const override;
	virtual std::string getFlavorText() const override;
};

#endif
