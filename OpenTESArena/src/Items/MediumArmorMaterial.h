#ifndef MEDIUM_ARMOR_MATERIAL_H
#define MEDIUM_ARMOR_MATERIAL_H

#include <string>

#include "ArmorMaterial.h"

// For chain armor only.
class MediumArmorMaterial : public ArmorMaterial
{
public:
	MediumArmorMaterial() = default;
	virtual ~MediumArmorMaterial() = default;

	virtual std::unique_ptr<ArmorMaterial> clone() const override;

	virtual ArmorMaterialType getMaterialType() const override;
	virtual int getArmorRating() const override;
	virtual int getConditionMultiplier() const override;

	// Compared to iron.
	virtual double getWeightMultiplier() const override;

	virtual bool isEnchantable() const override;
	virtual std::string toString() const override;
};

#endif
