#ifndef HEAVY_ARMOR_MATERIAL_H
#define HEAVY_ARMOR_MATERIAL_H

#include <string>

#include "ArmorMaterial.h"
#include "Metallic.h"

// For plate armor only.

class HeavyArmorMaterial : public ArmorMaterial, public Metallic
{
public:
	HeavyArmorMaterial(MetalType metalType);
	virtual ~HeavyArmorMaterial();

	virtual std::unique_ptr<ArmorMaterial> clone() const override;

	virtual ArmorMaterialType getMaterialType() const override;
	virtual int getArmorRating() const override;
	virtual int getConditionMultiplier() const override;
	virtual double getWeightMultiplier() const override;
	virtual std::string toString() const override;
};

#endif