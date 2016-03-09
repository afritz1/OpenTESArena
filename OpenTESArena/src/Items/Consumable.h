#ifndef CONSUMABLE_H
#define CONSUMABLE_H

#include <string>

#include "Item.h"

// A consumable is an abstract type because it can either be a food or a potion.
// While they don't have very different functionality, they do have slightly 
// different rules. Food doesn't need identification, while potions do, for example.
// This separation will keep foods from needing to implement any identification code.

enum class ConsumableType;

class Consumable : public Item
{
public:
	Consumable();
	virtual ~Consumable();

	virtual ItemType getItemType() const override;
	virtual double getWeight() const = 0;
	virtual int getGoldValue() const = 0;
	virtual std::string getDisplayName() const = 0;

	virtual ConsumableType getConsumableType() const = 0;
	virtual std::string typeToString() const = 0;

	// All consumables will have an "effect"; that effect could be changes to an 
	// attribute, or even a world function like teleportation. When implementing 
	// an "Effect" class, it won't be enough to only include the consumer as the 
	// thing being changed.
};

#endif