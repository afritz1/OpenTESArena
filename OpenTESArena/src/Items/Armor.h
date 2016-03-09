#ifndef ARMOR_PIECE_H
#define ARMOR_PIECE_H

#include <vector>

#include "Item.h"

// Inherit this class for any armor-like type that can be split into multiple
// body-part-oriented pieces.

// I don't like that the Shield type is included here, because while the body parts
// method is overridden in the Shield class, it has a dummy value stuck in one of 
// the mappings in this class. In order to fix that, I think I'd need another 
// abstract class that the light and heavy armors inherit from that the Shield class 
// doesn't, and I'd also need to take the Shield element out of the ArmorPieceType 
// enum. There's nothing that comes to mind which sufficiently differentiates them, 
// because a shield IS a type of armor piece, so I'll just leave it as is for now.

// -----------------------------------------

// Just like "ArmorMaterial", the Armor class suffers from the same mix of concrete
// armors (helm, cuirass, boots) and abstract armors (shields). That means that this
// class will need to become polymorphic.

class ArmorMaterial;

enum class ArmorType;
enum class BodyPartName;

class Armor : public Item
{
public:
	Armor();
	virtual ~Armor();

	virtual ItemType getItemType() const override;
	virtual double getWeight() const = 0;
	virtual int getGoldValue() const = 0;
	virtual std::string getDisplayName() const = 0;

	virtual ArmorType getArmorType() const = 0;

	// This is virtual because shields only have "HeavyArmorMaterial", while body 
	// armor can have any kind.
	virtual const ArmorMaterial *getArmorMaterial() const = 0;

	virtual std::vector<BodyPartName> getProtectedBodyParts() const = 0;
	virtual int getArmorRating() const = 0;
};

#endif