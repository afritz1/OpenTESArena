#ifndef ARMOR_PIECE_H
#define ARMOR_PIECE_H

#include <vector>

#include "Item.h"

// This abstract class is implemented by the BodyArmor (cuirass, helmet, legs, etc.) 
// class and the Shield class.

class ArmorMaterial;
class ArtifactData;

enum class ArmorType;
enum class BodyPartName;

class Armor : public Item
{
public:
	Armor(const ArtifactData *artifactData);
	virtual ~Armor() = default;

	virtual std::unique_ptr<Item> clone() const override = 0;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override = 0;
	virtual int getGoldValue() const override = 0;
	virtual std::string getDisplayName() const override = 0;

	virtual ArmorType getArmorType() const = 0;

	// This is virtual because shields only have "HeavyArmorMaterial", while body 
	// armor can have any kind of armor material.
	virtual const ArmorMaterial *getArmorMaterial() const = 0;

	// A list of the body parts a piece of armor protects.
	virtual std::vector<BodyPartName> getProtectedBodyParts() const = 0;

	virtual int getArmorRating() const = 0;
};

#endif
