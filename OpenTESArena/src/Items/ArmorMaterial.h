#ifndef ARMOR_MATERIAL_H
#define ARMOR_MATERIAL_H

#include <memory>
#include <string>

// It's not that an armor piece is light or heavy, but its MATERIAL is light
// or heavy. That's the behavior I was looking for.

// Having every armor piece inherit a material class allows for some polymorphic
// features like heavy armor materials having a metal type.

enum class ArmorMaterialType;

class ArmorMaterial
{
public:
	ArmorMaterial();
	virtual ~ArmorMaterial();

	// I'm not sure how else to get a mapping from ArmorMaterialType to string
	// because there isn't one for "Plate" when all heavy armors have their own
	// specific name. Somehow homogenize all this "toString()" stuff sometime.
	static std::string typeToString(ArmorMaterialType materialType);

	virtual std::unique_ptr<ArmorMaterial> clone() const = 0;

	virtual ArmorMaterialType getMaterialType() const = 0;
	virtual int getArmorRating() const = 0;
	virtual int getConditionMultiplier() const = 0; // Metal materials are stronger.
	virtual double getWeightMultiplier() const = 0; // Some materials are heavier.
	virtual bool isEnchantable() const = 0;
	virtual std::string toString() const = 0;
};

#endif
