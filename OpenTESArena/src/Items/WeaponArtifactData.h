#ifndef WEAPON_ARTIFACT_DATA_H
#define WEAPON_ARTIFACT_DATA_H

#include <memory>

#include "ArtifactData.h"

enum class ItemType;
enum class MetalType;

class WeaponArtifactData : public ArtifactData
{
private:
	int weaponID;
	MetalType metalType;
public:
	WeaponArtifactData(const std::string &displayName,
		const std::string &flavorText, const std::vector<int> &provinceIDs, 
		int weaponID, MetalType metalType);
	virtual ~WeaponArtifactData();

	virtual std::unique_ptr<ArtifactData> clone() const override;

	int getWeaponID() const;
	MetalType getMetalType() const;

	virtual ItemType getItemType() const override;
};

#endif
