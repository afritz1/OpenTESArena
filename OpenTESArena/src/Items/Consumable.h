#ifndef CONSUMABLE_H
#define CONSUMABLE_H

#include <string>

#include "Item.h"

enum class ConsumableType;

class Consumable : public Item
{
public:
	// There are no consumable artifacts yet, so this constructor remains simple.
	Consumable();
	virtual ~Consumable() = default;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override = 0;
	virtual int getGoldValue() const override = 0;
	virtual std::string getDisplayName() const override = 0;

	virtual ConsumableType getConsumableType() const = 0;
	virtual std::string typeToString() const = 0;

	// All consumables will have an "effect"; that effect could be changes to an 
	// attribute, or even a world function like teleportation. When implementing 
	// an "Effect" class, it won't be enough to only include the consumer as the 
	// thing being changed. A pointer to the GameState might be required.
	// const Effect &getEffect() const...
};

#endif
