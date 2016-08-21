#ifndef MEDIUM_ARMOR_MATERIAL_H
#define MEDIUM_ARMOR_MATERIAL_H

#include <string>

#include "ArmorMaterial.h"

// For chain armor only.

class MediumArmorMaterial : public ArmorMaterial
{
public:
	MediumArmorMaterial();
	virtual ~MediumArmorMaterial();

	virtual std::unique_ptr<ArmorMaterial> clone() const override;

	virtual ArmorMaterialType getMaterialType() const override;
	virtual int32_t getArmorRating() const override;
	virtual int32_t getConditionMultiplier() const override;

	// Compared to iron.
	virtual double getWeightMultiplier() const override;

	virtual bool isEnchantable() const override;
	virtual std::string toString() const override;
};

#endif
