#ifndef LIGHT_ARMOR_MATERIAL_H
#define LIGHT_ARMOR_MATERIAL_H

#include <string>

#include "ArmorMaterial.h"

// For leather armor only.

class LightArmorMaterial : public ArmorMaterial
{
public:
	LightArmorMaterial();
	virtual ~LightArmorMaterial();

	virtual std::unique_ptr<ArmorMaterial> clone() const override;

	virtual ArmorMaterialType getMaterialType() const override;
	virtual int getArmorRating() const override;
	virtual int getConditionMultiplier() const override;

	// Compared to iron.
	virtual double getWeightMultiplier() const override;

	virtual std::string toString() const override;
};

#endif