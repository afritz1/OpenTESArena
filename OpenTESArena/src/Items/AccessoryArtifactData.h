#ifndef ACCESSORY_ARTIFACT_DATA_H
#define ACCESSORY_ARTIFACT_DATA_H

#include <string>

#include "AccessoryArtifactName.h"
#include "ArtifactData.h"

enum class AccessoryType;
enum class MetalType;

class AccessoryArtifactData : public ArtifactData
{
private:
	AccessoryArtifactName artifactName;
public:
	AccessoryArtifactData(AccessoryArtifactName artifactName);
	virtual ~AccessoryArtifactData();

	const AccessoryArtifactName &getArtifactName() const;
	AccessoryType getAccessoryType() const;
	MetalType getMetalType() const;
	virtual ArtifactName getParentArtifactName() const override;
	virtual std::string getDisplayName() const override;
	virtual std::string getFlavorText() const override;
};

#endif