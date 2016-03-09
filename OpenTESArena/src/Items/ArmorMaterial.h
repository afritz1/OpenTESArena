#ifndef ARMOR_MATERIAL_H
#define ARMOR_MATERIAL_H

#include <memory>
#include <string>

#include "ArmorMaterialType.h"

// It's not that the armor piece is light or heavy, but its MATERIAL is light
// or heavy. That's the behavior I was looking for.

// Having every armor piece inherit a material class allows for some polymorphic
// features like heavy armor materials having a metal type.

enum class MetalType;

class ArmorMaterial
{
public:
	ArmorMaterial();
	virtual ~ArmorMaterial();

	virtual std::unique_ptr<ArmorMaterial> clone() const = 0;

	virtual ArmorMaterialType getMaterialType() const = 0;
	virtual int getArmorRating() const = 0;
	virtual int getConditionMultiplier() const = 0; // Metal materials are stronger.
	virtual double getWeightMultiplier() const = 0; // Some materials are heavier.
	virtual std::string toString() const = 0;
};

#endif