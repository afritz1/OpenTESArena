#ifndef WEAPON_ARTIFACT_DATA_H
#define WEAPON_ARTIFACT_DATA_H

#include <memory>

#include "ArtifactData.h"

enum class ItemType;
enum class MetalType;
enum class WeaponType;

class WeaponArtifactData : public ArtifactData
{
private:
	WeaponType weaponType;
	MetalType metalType;
public:
	WeaponArtifactData(const std::string &displayName,
		const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
		const WeaponType &weaponType, const MetalType &metalType);
	virtual ~WeaponArtifactData();

	virtual std::unique_ptr<ArtifactData> clone() const override;

	const WeaponType &getWeaponType() const;
	const MetalType &getMetalType() const;

	virtual ItemType getItemType() const override;
};

#endif
